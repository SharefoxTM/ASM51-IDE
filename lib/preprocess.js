"use strict";
// Translates the course dialect (Reads51 style) into plain Intel syntax for ASEM-51.
// Only whole tokens are rewritten, so line numbers stay identical to the source.
const fs = require("fs");
const path = require("path");

const DEFINERS = "equ|bit|data|code|xdata|idata|set|reg|segment|sfr|sbit";
const LABEL_RE = /^\s*([A-Za-z_?][\w?]*)\s*:/;
const DEFINE_RE = new RegExp("^\\s*([A-Za-z_?][\\w?]*)\\s+(?:" + DEFINERS + ")\\b", "i");
const HOIST_RE = /^\s*[A-Za-z_?][\w?]*\s+(?:data|bit|sfr|sbit|equ)\s+(?:[0-9][0-9a-fA-F]*[hH]?|[0-9a-fA-F]+[hH])\s*$/i;
const INCLUDE_RE = /^(\s*)#\s*include\s*(?:"([^"]+)"|<([^>]+)>|(\S+))\s*(;.*)?$/i;
// A hex literal that forgot the leading digit, e.g. #efh
const BAD_HEX_RE = /(?<![\w.$?@])([a-f][0-9a-f]*h)(?![\w?@])/gi;

function splitComment(line) {
	let quote = null;
	for (let i = 0; i < line.length; i++) {
		const c = line[i];
		if (quote) {
			if (c === quote) {
				quote = null;
			}
		} else if (c === "'" || c === '"') {
			quote = c;
		} else if (c === ";") {
			return [line.slice(0, i), line.slice(i)];
		}
	}
	return [line, ""];
}

function fixHex(code, symbols) {
	const parts = code.split(/('[^']*'|"[^"]*")/);
	return parts
		.map((part, i) =>
			i % 2 === 1
				? part
				: part.replace(BAD_HEX_RE, (tok) => (symbols.has(tok.toLowerCase()) ? tok : "0" + tok)),
		)
		.join("");
}

// ASEM-51 directives that Reads51 accepts as user symbols (e.g. a label called "name")
const RESERVED = new Set(["name", "public", "extrn", "using", "bseg", "cseg", "dseg", "iseg", "xseg", "rseg", "stack"]);
const RESERVED_RE = /(?<![\w.$?@])([A-Za-z]+)(?![\w?@])/g;

function renameReserved(code, symbols) {
	const parts = code.split(/('[^']*'|"[^"]*")/);
	return parts
		.map((part, i) =>
			i % 2 === 1
				? part
				: part.replace(RESERVED_RE, (tok) =>
						RESERVED.has(tok.toLowerCase()) && symbols.has(tok.toLowerCase()) ? tok + "_" : tok,
					),
		)
		.join("");
}

function findInsensitive(file) {
	if (fs.existsSync(file)) {
		return file;
	}
	const dir = path.dirname(file);
	try {
		const hit = fs.readdirSync(dir).find((n) => n.toLowerCase() === path.basename(file).toLowerCase());
		return hit ? path.join(dir, hit) : undefined;
	} catch {
		return undefined;
	}
}

function resolveInclude(name, fromDir, includeDirs) {
	const candidates = [];
	const unix = name.replace(/\\/g, "/");
	const base = path.posix.basename(unix);
	if (/^[A-Za-z]:\//.test(unix)) {
		// Windows drive path: only usable on Windows, elsewhere fall back to the search folders
		if (process.platform === "win32") {
			candidates.push(name);
		}
	} else if (path.isAbsolute(unix)) {
		candidates.push(unix);
	} else {
		candidates.push(path.resolve(fromDir, unix));
	}
	candidates.push(path.resolve(fromDir, base));
	for (const dir of includeDirs) {
		candidates.push(path.join(dir, base));
	}
	for (const c of candidates) {
		const hit = findInsensitive(c);
		if (hit && fs.statSync(hit).isFile()) {
			return hit;
		}
	}
	return undefined;
}

function readText(file) {
	return fs.readFileSync(file).toString("latin1");
}

/**
 * Preprocesses `mainFile` and every file it includes into `outDir`.
 * Returns { main, files, missing } where `files` maps generated name -> original path.
 */
function preprocess(mainFile, outDir, includeDirs) {
	const files = new Map(); // original abs path -> generated name
	const queue = [];
	const missing = [];
	const symbols = new Set();
	const sources = new Map();

	const generatedName = (original, isMain) => {
		if (files.has(original)) {
			return files.get(original);
		}
		const name = isMain ? "main.asm" : "inc" + files.size + "_" + path.basename(original).replace(/[^\w.]/g, "_");
		files.set(original, name);
		queue.push(original);
		return name;
	};

	generatedName(path.resolve(mainFile), true);
	// Pass 1: discover includes and defined symbols
	for (let n = 0; n < queue.length; n++) {
		const original = queue[n];
		const text = readText(original);
		sources.set(original, text);
		for (const line of text.split(/\r?\n/)) {
			const m = line.match(INCLUDE_RE);
			if (m) {
				const target = resolveInclude(m[2] || m[3] || m[4], path.dirname(original), includeDirs);
				if (target) {
					generatedName(path.resolve(target), false);
				}
				continue;
			}
			const [code] = splitComment(line);
			const d = code.match(LABEL_RE) || code.match(DEFINE_RE);
			if (d) {
				symbols.add(d[1].toLowerCase());
			}
		}
	}

	// Pass 2: rewrite
	fs.mkdirSync(outDir, { recursive: true });
	// Include files that define their own SFRs clash with ASEM's built-in 8051 symbols
	const noMod51 = symbols.has("p0") || symbols.has("acc");
	const hoisted = [];
	const result = { main: path.join(outDir, "main.asm"), files: {}, missing, lineOffset: noMod51 ? 2 : 0 };
	for (const [original, name] of files) {
		const out = sources
			.get(original)
			.split(/\r?\n/)
			.map((line, idx) => {
				const m = line.match(INCLUDE_RE);
				if (m) {
					const wanted = m[2] || m[3] || m[4];
					const target = resolveInclude(wanted, path.dirname(original), includeDirs);
					if (!target) {
						missing.push({ file: original, line: idx + 1, name: wanted });
						return line;
					}
					return `${m[1]}$include(${files.get(path.resolve(target))})${m[5] ? " " + m[5] : ""}`;
				}
				const [code, comment] = splitComment(line);
				const fixed = renameReserved(fixHex(code, symbols), symbols);
				// ASEM needs SFR/bit definitions before their first use; the course include defines them at the end
				if (noMod51 && name !== "main.asm" && HOIST_RE.test(fixed)) {
					hoisted.push(fixed);
					return "";
				}
				return fixed + comment;
			})
			.join("\r\n");
		const header = noMod51 && name === "main.asm" ? "$NOMOD51\r\n$include(hoist.inc)\r\n" : "";
		fs.writeFileSync(path.join(outDir, name), header + out, "latin1");
		result.files[name] = original;
	}
	if (noMod51) {
		fs.writeFileSync(path.join(outDir, "hoist.inc"), hoisted.join("\r\n") + "\r\n", "latin1");
	}
	return result;
}

module.exports = { preprocess, fixHex };
