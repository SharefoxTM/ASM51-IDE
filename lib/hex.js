"use strict";
// Intel HEX parser and HTML memory-dump renderer.

function parseHex(text) {
	const bytes = new Map();
	const errors = [];
	let base = 0;
	let eof = false;
	let start;
	const lines = text.split(/\r?\n/);
	lines.forEach((raw, i) => {
		const line = raw.trim();
		if (!line) {
			return;
		}
		const err = (msg) => errors.push({ line: i + 1, message: msg });
		if (line[0] !== ":" || !/^:([0-9a-fA-F]{2})+$/.test(line)) {
			return err("Not a valid Intel HEX record");
		}
		const b = [];
		for (let p = 1; p < line.length; p += 2) {
			b.push(parseInt(line.slice(p, p + 2), 16));
		}
		const len = b[0];
		if (b.length !== len + 5) {
			return err(`Record length mismatch (declared ${len} data bytes)`);
		}
		if (b.reduce((s, v) => s + v, 0) % 256 !== 0) {
			err("Checksum error");
		}
		const addr = (b[1] << 8) | b[2];
		const type = b[3];
		const data = b.slice(4, 4 + len);
		if (eof) {
			err("Data after end-of-file record");
		}
		switch (type) {
			case 0:
				data.forEach((v, k) => {
					const a = base + addr + k;
					if (bytes.has(a)) {
						err(`Address ${a.toString(16).toUpperCase()}h written twice`);
					}
					bytes.set(a, v);
				});
				break;
			case 1:
				eof = true;
				break;
			case 2:
				base = ((data[0] << 8) | data[1]) * 16;
				break;
			case 4:
				base = ((data[0] << 8) | data[1]) * 65536;
				break;
			case 3:
			case 5:
				start = data.reduce((s, v) => s * 256 + v, 0);
				break;
			default:
				err(`Unknown record type ${type}`);
		}
	});
	if (!eof) {
		errors.push({ line: lines.length, message: "Missing end-of-file record" });
	}
	return { bytes, errors, start };
}

function ranges(bytes) {
	const addrs = [...bytes.keys()].sort((a, b) => a - b);
	const out = [];
	for (const a of addrs) {
		const last = out[out.length - 1];
		if (last && a === last.end + 1) {
			last.end = a;
		} else {
			out.push({ start: a, end: a });
		}
	}
	return out;
}

const esc = (s) => String(s).replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" })[c]);
const h = (n, w) => n.toString(16).toUpperCase().padStart(w, "0");

function renderHtml(text) {
	const { bytes, errors, start } = parseHex(text);
	const rs = ranges(bytes);
	const rows = [];
	let lastRow = -1;
	const rowKeys = [...new Set([...bytes.keys()].map((a) => a >> 4))].sort((a, b) => a - b);
	for (const row of rowKeys) {
		if (lastRow !== -1 && row !== lastRow + 1) {
			rows.push('<tr class="gap"><td colspan="3">…</td></tr>');
		}
		lastRow = row;
		let cells = "";
		let ascii = "";
		for (let c = 0; c < 16; c++) {
			const a = row * 16 + c;
			const v = bytes.get(a);
			const sep = c === 8 ? " sp" : "";
			if (v === undefined) {
				cells += `<span class="b none${sep}">--</span>`;
				ascii += " ";
			} else {
				cells += `<span class="b${v === 0xff ? " ff" : ""}${sep}" title="${h(a, 4)}h = ${v} (${v.toString(2).padStart(8, "0")}b)">${h(v, 2)}</span>`;
				ascii += v >= 32 && v < 127 ? esc(String.fromCharCode(v)) : "·";
			}
		}
		rows.push(`<tr><td class="addr">${h(row * 16, 4)}</td><td>${cells}</td><td class="asc">${ascii}</td></tr>`);
	}
	const total = bytes.size;
	const summary = rs.map((r) => `${h(r.start, 4)}h–${h(r.end, 4)}h (${r.end - r.start + 1} bytes)`).join(", ") || "no data";
	const problems = errors.length
		? `<div class="err"><b>${errors.length} problem(s)</b><ul>${errors
				.slice(0, 50)
				.map((e) => `<li>line ${e.line}: ${esc(e.message)}</li>`)
				.join("")}</ul></div>`
		: '<div class="ok">All records valid, checksums OK.</div>';
	return `<!DOCTYPE html><html><head><meta charset="utf-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline';">
<style>
body{font-family:var(--vscode-editor-font-family,monospace);font-size:var(--vscode-editor-font-size,13px);color:var(--vscode-editor-foreground);padding:8px 16px}
table{border-collapse:collapse}td{padding:1px 10px 1px 0;white-space:pre}
.addr{color:var(--vscode-editorLineNumber-foreground)}
.b{display:inline-block;width:2.2ch;margin-right:.9ch}.b.sp{margin-left:1.5ch}
.b.ff{opacity:.45}.b.none{opacity:.2}.b:hover{background:var(--vscode-editor-selectionBackground)}
.asc{color:var(--vscode-descriptionForeground)}.gap td{opacity:.4}
.err{color:var(--vscode-errorForeground)}.ok{color:var(--vscode-testing-iconPassed,#3c3)}
.sum{margin:6px 0 12px;opacity:.85}
</style></head><body>
<div class="sum">Intel HEX · ${total} bytes · ${esc(summary)}${start !== undefined ? ` · entry ${h(start, 4)}h` : ""}</div>
${problems}
<table>${rows.join("")}</table></body></html>`;
}

const FLASH_LIMIT = 0xf800; // ADuC832: 62 KB user flash, the serial loader lives above it

// Rebuilds an Intel HEX image the way the lsd downloader wants it: address-ordered, gaps and
// padding filled with 0xFF, 16-byte records from address 0, whole 256-byte pages.
function toFlashHex(text) {
	const { bytes, errors } = parseHex(text);
	if (errors.length) {
		throw new Error(`Hex file is invalid: line ${errors[0].line}: ${errors[0].message}`);
	}
	if (bytes.size === 0) {
		throw new Error("Hex file contains no data.");
	}
	const max = Math.max(...bytes.keys());
	if (max >= FLASH_LIMIT) {
		throw new Error(`Program reaches address ${max.toString(16).toUpperCase()}h, beyond the ${FLASH_LIMIT / 1024} KB of ADuC832 user flash.`);
	}
	const size = Math.ceil((max + 1) / 256) * 256;
	const image = Buffer.alloc(size, 0xff);
	for (const [a, v] of bytes) {
		image[a] = v;
	}
	const lines = [];
	for (let a = 0; a < size; a += 16) {
		const rec = [16, a >> 8, a & 0xff, 0, ...image.subarray(a, a + 16)];
		const sum = rec.reduce((s, b) => s + b, 0);
		lines.push(":" + [...rec, (-sum) & 0xff].map((b) => b.toString(16).toUpperCase().padStart(2, "0")).join(""));
	}
	lines.push(":00000001FF");
	return { hex: lines.join("\n") + "\n", size, pages: size / 256, used: bytes.size };
}

module.exports = { parseHex, ranges, renderHtml, toFlashHex };
