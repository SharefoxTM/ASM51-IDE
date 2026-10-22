"use strict";
const vscode = require("vscode");
const fs = require("fs");
const path = require("path");
const cp = require("child_process");
const { preprocess } = require("./lib/preprocess");
const { renderHtml, toFlashHex } = require("./lib/hex");
const flasher = require("./lib/flasher");

const ASEM_ZIP = "asem5113.zip";
const ASEM_EXE = "ASEMW.EXE";
const ASEM_TAR = "asem51-1.3-ELF.tar.gz";
const ASEM_LINUX = "asem";

let output;
let diagnostics;
let busy = false;

function activate(context) {
	output = vscode.window.createOutputChannel("ASM51");
	diagnostics = vscode.languages.createDiagnosticCollection("asm51");

	const compileItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 1000);
	compileItem.text = "$(play) ASM51 Compile";
	compileItem.tooltip = "Compile the active file with ASEM-51 (F7)";
	compileItem.command = "asm51.compile";
	const flashItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 999);
	flashItem.text = "$(zap) Flash";
	flashItem.tooltip = "Compile and flash the active file (F8)";
	flashItem.command = "asm51.flash";
	const portItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 998);
	portItem.command = "asm51.selectPort";
	portItem.tooltip = "Select the serial port used for flashing";

	const refreshStatus = () => {
		const editor = vscode.window.activeTextEditor;
		const visible = editor && editor.document.languageId === "asm51" && editor.document.uri.scheme === "file";
		for (const item of [compileItem, flashItem, portItem]) {
			visible ? item.show() : item.hide();
		}
		portItem.text = "$(plug) " + (currentPort(context) || "Port: ask");
	};
	refreshStatus();

	context.subscriptions.push(
		output,
		diagnostics,
		compileItem,
		flashItem,
		portItem,
		vscode.window.onDidChangeActiveTextEditor(refreshStatus),
		vscode.workspace.onDidChangeConfiguration(refreshStatus),
		vscode.commands.registerCommand("asm51.compile", () => run(context, false)),
		vscode.commands.registerCommand("asm51.flash", () => run(context, true)),
		vscode.commands.registerCommand("asm51.checkInstall", () => runCheckInstall(context, true)),
		vscode.commands.registerCommand("asm51.selectPort", async () => {
			await pickPort(context, true);
			refreshStatus();
		}),
		vscode.workspace.onDidSaveTextDocument((doc) => {
			const auto = vscode.workspace.getConfiguration("asm51").get("compileOnSave");
			if (auto && doc.languageId === "asm51" && /\.(asm|a51)$/i.test(doc.fileName)) {
				run(context, false, doc);
			}
		}),
		vscode.workspace.onDidCloseTextDocument((doc) => diagnostics.delete(doc.uri)),
		vscode.window.registerCustomEditorProvider(
			"asm51.hexViewer",
			{
				resolveCustomTextEditor(document, panel) {
					const render = () => (panel.webview.html = renderHtml(document.getText()));
					render();
					const sub = vscode.workspace.onDidChangeTextDocument((e) => {
						if (e.document.uri.toString() === document.uri.toString()) {
							render();
						}
					});
					panel.onDidDispose(() => sub.dispose());
				},
			},
			{ supportsMultipleEditorsPerDocument: true },
		),
		vscode.commands.registerCommand("asm51.viewHex", (uri) => {
			const target = uri || (vscode.window.activeTextEditor && vscode.window.activeTextEditor.document.uri);
			if (!target || !/\.hex$/i.test(target.fsPath)) {
				vscode.window.showWarningMessage("Open a .hex file first.");
				return;
			}
			return vscode.commands.executeCommand("vscode.openWith", target, "asm51.hexViewer");
		}),
	);

	// Verify the installation once per version (and again after every failed check).
	if (context.globalState.get("asm51.checkedVersion") !== context.extension.packageJSON.version) {
		runCheckInstall(context, false).catch((e) => output.appendLine("Installation check crashed: " + e));
	}
}

function currentPort(context) {
	const configured = vscode.workspace.getConfiguration("asm51").get("flash.port");
	return configured || context.globalState.get("asm51.lastPort") || "";
}

async function run(context, flash, document) {
	const editor = vscode.window.activeTextEditor;
	const doc = document || (editor && editor.document);
	if (!doc || doc.languageId !== "asm51" || doc.uri.scheme !== "file" || !/\.(asm|a51)$/i.test(doc.fileName)) {
		vscode.window.showWarningMessage("Open an 8051 assembly file (.asm) first.");
		return;
	}
	if (busy) {
		return;
	}
	busy = true;
	try {
		if (doc.isDirty) {
			await doc.save();
		}
		const result = await compile(context, doc.uri.fsPath);
		if (result.ok && flash) {
			await flashHex(context, result.hex, doc.uri.fsPath);
		}
	} catch (err) {
		output.appendLine("ERROR: " + (err && err.message ? err.message : err));
		output.show(true);
		vscode.window.showErrorMessage("ASM51: " + (err && err.message ? err.message : err));
	} finally {
		busy = false;
	}
}

function ensureAssembler(context) {
	const isWin = process.platform === "win32";
	if (!isWin && process.platform !== "linux") {
		throw new Error("Only Windows and Linux (x86/x86_64) are supported by the bundled ASEM-51 assembler.");
	}
	if (!isWin && !["x64", "ia32"].includes(process.arch)) {
		throw new Error(
			`The bundled ASEM-51 is an x86 program and cannot run on ${process.arch}. Set 'asm51.assemblerPath' to an ASEM-51 binary for your system.`,
		);
	}
	const custom = vscode.workspace.getConfiguration("asm51").get("assemblerPath");
	if (custom) {
		return custom;
	}
	const dir = path.join(context.globalStorageUri.fsPath, "asem51");
	const exe = path.join(dir, isWin ? ASEM_EXE : ASEM_LINUX);
	if (!fs.existsSync(exe)) {
		fs.mkdirSync(dir, { recursive: true });
		const vendor = path.join(context.extensionPath, "vendor");
		const r = isWin
			? cp.spawnSync(
					"powershell.exe",
					["-NoProfile", "-NonInteractive", "-Command", "Expand-Archive -LiteralPath $env:ASM51_ZIP -DestinationPath $env:ASM51_DEST -Force"],
					{ encoding: "utf8", env: { ...process.env, ASM51_ZIP: path.join(vendor, ASEM_ZIP), ASM51_DEST: dir } },
				)
			: cp.spawnSync("tar", ["-xzf", path.join(vendor, ASEM_TAR), "-C", dir, "--strip-components=1"], { encoding: "utf8" });
		if (r.status !== 0 || !fs.existsSync(exe)) {
			throw new Error("Could not unpack ASEM-51: " + (r.stderr || r.error || "unknown error"));
		}
		if (!isWin) {
			fs.chmodSync(exe, 0o755);
		}
	}
	return exe;
}

function execFile(file, args, options) {
	return new Promise((resolve, reject) => {
		const child = cp.spawn(file, args, { ...options, windowsHide: true });
		let out = "";
		child.stdout.on("data", (d) => (out += d.toString("latin1")));
		child.stderr.on("data", (d) => (out += d.toString("latin1")));
		child.on("error", reject);
		child.on("close", (code) => resolve({ code, out }));
	});
}

async function compile(context, source) {
	const asem = ensureAssembler(context);
	const work = path.join(context.globalStorageUri.fsPath, "work");
	fs.rmSync(work, { recursive: true, force: true });

	const cfg = vscode.workspace.getConfiguration("asm51");
	const includeDirs = [
		path.dirname(source),
		...cfg.get("includePaths"),
		...(vscode.workspace.workspaceFolders || []).map((f) => f.uri.fsPath),
		path.join(context.extensionPath, "inc"),
	];
	const pre = preprocess(source, work, includeDirs);

	output.clear();
	output.appendLine(`Assembling ${source}`);
	const { code, out } = await execFile(asem, ["main.asm", "main.hex", "main.lst"], { cwd: work });
	output.appendLine(out.trim());

	const perFile = new Map();
	const addDiag = (file, line, message, severity) => {
		const uri = vscode.Uri.file(file);
		const list = perFile.get(uri.toString()) || { uri, items: [] };
		const n = Math.max(0, line - 1);
		const range = new vscode.Range(n, 0, n, Number.MAX_SAFE_INTEGER);
		list.items.push(new vscode.Diagnostic(range, message, severity));
		perFile.set(uri.toString(), list);
	};

	for (const m of pre.missing) {
		addDiag(m.file, m.line, `Include file not found: ${m.name}`, vscode.DiagnosticSeverity.Error);
	}
	let errors = pre.missing.length;
	for (const raw of out.split(/\r?\n/)) {
		const m = raw.match(/^(\S+?)\((\d+)\):\s*(.*)$/);
		if (!m) {
			continue;
		}
		const original = pre.files[m[1]] || source;
		const offset = m[1] === "main.asm" ? pre.lineOffset : 0;
		const isWarning = /warning/i.test(m[3]);
		isWarning || errors++;
		addDiag(
			original,
			Number(m[2]) - offset,
			m[3].replace(/^warning:\s*/i, ""),
			isWarning ? vscode.DiagnosticSeverity.Warning : vscode.DiagnosticSeverity.Error,
		);
	}

	diagnostics.clear();
	for (const { uri, items } of perFile.values()) {
		diagnostics.set(uri, items);
	}

	const ok = code === 0 && errors === 0;
	const base = source.replace(/\.[^.\\/]*$/, "");
	if (ok) {
		fs.copyFileSync(path.join(work, "main.hex"), base + ".hex");
		fs.copyFileSync(path.join(work, "main.lst"), base + ".lst");
		output.appendLine(`OK -> ${base}.hex`);
		vscode.window.setStatusBarMessage("$(check) ASM51: build OK", 4000);
	} else {
		output.show(true);
		vscode.commands.executeCommand("workbench.actions.view.problems");
		vscode.window.setStatusBarMessage("$(error) ASM51: build failed", 6000);
	}
	return { ok, hex: base + ".hex" };
}

async function listPorts() {
	if (process.platform === "win32") {
		const script =
			"Get-CimInstance Win32_PnPEntity | Where-Object { $_.Name -match '\\(COM\\d+\\)' } | ForEach-Object { $_.Name }";
		const { out } = await execFile("powershell.exe", ["-NoProfile", "-NonInteractive", "-Command", script], {});
		return out
			.split(/\r?\n/)
			.map((l) => l.trim())
			.filter(Boolean)
			.map((l) => ({ port: l.match(/\((COM\d+)\)/)[1], label: l }));
	}
	const dir = "/dev";
	return fs
		.readdirSync(dir)
		.filter((n) => /^(ttyUSB|ttyACM|tty\.usb|cu\.usb)/.test(n))
		.map((n) => ({ port: path.join(dir, n), label: path.join(dir, n) }));
}

async function pickPort(context, force) {
	const configured = vscode.workspace.getConfiguration("asm51").get("flash.port");
	if (configured && !force) {
		return configured;
	}
	const ports = await listPorts();
	if (ports.length === 0) {
		vscode.window.showWarningMessage("No serial ports found. Connect the board and try again.");
		return undefined;
	}
	const last = context.globalState.get("asm51.lastPort");
	if (!force && last && ports.some((p) => p.port === last) && ports.length === 1) {
		return last;
	}
	const pick = await vscode.window.showQuickPick(
		ports.map((p) => ({ label: p.port, description: p.label, picked: p.port === last })),
		{ placeHolder: "Select the serial port of the board" },
	);
	if (!pick) {
		return undefined;
	}
	await context.globalState.update("asm51.lastPort", pick.label);
	return pick.label;
}

async function flashHex(context, hex, source) {
	const cfg = vscode.workspace.getConfiguration("asm51");
	const custom = cfg.get("flash.toolPath");
	if (custom) {
		return flashWithCustomTool(context, custom, hex, source);
	}

	// lsd ignores record addresses and drops partial pages, so hand it a normalised image.
	const image = toFlashHex(fs.readFileSync(hex, "utf8"));
	const port = await pickPort(context, false);
	if (!port) {
		return;
	}
	const hint = flasher.linuxPortHint(port);
	if (hint) {
		throw new Error(hint);
	}
	const tool = flasher.prepare(context.extensionPath, context.globalStorageUri.fsPath);
	const work = path.join(context.globalStorageUri.fsPath, "work");
	fs.mkdirSync(work, { recursive: true });
	const flashFile = path.join(work, "flash.hex");
	fs.writeFileSync(flashFile, image.hex);

	const inv = flasher.invocation(tool, { port, hex: flashFile, run: cfg.get("flash.run") });
	output.appendLine(`Flashing ${image.used} bytes (${image.pages} pages) to ${port}`);
	output.appendLine(`> ${inv.command} ${inv.args.join(" ")}`);
	output.show(true);
	await vscode.window.withProgress({ location: vscode.ProgressLocation.Notification, title: "ASM51: flashing…" }, async () => {
		const result = await flasher.runFlash(inv, { onData: (t) => output.append(t) });
		output.appendLine("");
		if (result.ok) {
			output.appendLine("Flash OK");
			vscode.window.showInformationMessage("ASM51: flash completed.");
		} else {
			output.appendLine("Flash FAILED: " + result.message);
			vscode.window.showErrorMessage("ASM51: " + result.message);
		}
	});
}

async function flashWithCustomTool(context, tool, hex, source) {
	const args = vscode.workspace.getConfiguration("asm51").get("flash.args");
	let port = "";
	if (args.some((a) => a.includes("${port}"))) {
		port = await pickPort(context, false);
		if (!port) {
			return;
		}
	}
	const resolved = args.map((a) => a.replace(/\$\{hex\}/g, hex).replace(/\$\{port\}/g, port).replace(/\$\{file\}/g, source));
	output.appendLine(`Flashing: ${tool} ${resolved.join(" ")}`);
	output.show(true);
	await vscode.window.withProgress({ location: vscode.ProgressLocation.Notification, title: "ASM51: flashing…" }, async () => {
		const shell = /\.(cmd|bat)$/i.test(tool);
		const quote = (s) => (shell && /\s/.test(s) ? `"${s}"` : s);
		const { code, out } = await execFile(quote(tool), resolved.map(quote), { cwd: path.dirname(hex), shell });
		output.appendLine(out.trim());
		if (code === 0) {
			output.appendLine("Flash OK");
			vscode.window.showInformationMessage("ASM51: flash completed.");
		} else {
			vscode.window.showErrorMessage(`ASM51: flash tool failed (exit code ${code}). See the ASM51 output.`);
		}
	});
}

// Verifies every part of the installation by actually running it. Returns a list of {name, ok, detail}.
async function checkInstall(context) {
	const results = [];
	const step = async (name, fn) => {
		try {
			const detail = await fn();
			results.push({ name, ok: true, detail: detail || "" });
		} catch (e) {
			results.push({ name, ok: false, detail: e && e.message ? e.message : String(e) });
		}
	};
	const isWin = process.platform === "win32";

	await step("Platform", () => {
		if (!isWin && process.platform !== "linux") {
			throw new Error(`${process.platform} is not supported (Windows and Linux only).`);
		}
		return `${process.platform}/${process.arch}`;
	});

	if (!isWin) {
		await step("tar (needed to unpack the assembler)", () => {
			const r = cp.spawnSync("tar", ["--version"], { encoding: "utf8" });
			if (r.error || r.status !== 0) {
				throw new Error("'tar' was not found. Install it with your package manager, e.g. 'sudo apt install tar'.");
			}
			return r.stdout.split("\n")[0];
		});
	} else {
		await step("PowerShell (needed to unpack the assembler)", () => {
			const r = cp.spawnSync("powershell.exe", ["-NoProfile", "-NonInteractive", "-Command", "$PSVersionTable.PSVersion.ToString()"], {
				encoding: "utf8",
			});
			if (r.error || r.status !== 0) {
				throw new Error("powershell.exe could not be started: " + (r.error ? r.error.message : r.stderr));
			}
			return "PowerShell " + r.stdout.trim();
		});
	}

	await step("ASEM-51 assembler", async () => {
		const asem = ensureAssembler(context);
		const dir = path.join(context.globalStorageUri.fsPath, "selfcheck");
		fs.rmSync(dir, { recursive: true, force: true });
		fs.mkdirSync(dir, { recursive: true });
		fs.writeFileSync(path.join(dir, "t.asm"), "org 0\nmov a,#55h\nend\n");
		const { code, out } = await execFile(asem, ["t.asm", "t.hex", "t.lst"], { cwd: dir });
		const hex = path.join(dir, "t.hex");
		if (code !== 0 || !fs.existsSync(hex) || !/:020000007455/i.test(fs.readFileSync(hex, "latin1"))) {
			throw new Error(`Test assembly failed (exit ${code}): ${out.trim()}`);
		}
		fs.rmSync(dir, { recursive: true, force: true });
		return asem;
	});

	await step("Flash tool", async () => {
		if (vscode.workspace.getConfiguration("asm51").get("flash.toolPath")) {
			return "custom tool configured, not checked";
		}
		const tool = flasher.prepare(context.extensionPath, context.globalStorageUri.fsPath);
		if (tool.kind === "lsd") {
			const { code, out } = await execFile(tool.command, ["-h"], {});
			if (!/Usage: lsd/.test(out)) {
				throw new Error(`The flasher does not run on this system (exit ${code}): ${out.trim()}`);
			}
			return tool.command;
		}
		// Run the real script on a missing file: proves PowerShell policy allows it and that it executes.
		const inv = flasher.invocation(tool, { port: "COM1", hex: path.join(context.globalStorageUri.fsPath, "missing.hex"), run: false });
		const { out } = await execFile(inv.command, inv.args, {});
		if (!/Could not open file/.test(out)) {
			throw new Error("The flasher script could not run (blocked by PowerShell execution policy?): " + out.trim());
		}
		const ports = cp.spawnSync(
			"powershell.exe",
			["-NoProfile", "-NonInteractive", "-Command", "[System.IO.Ports.SerialPort]::GetPortNames() -join ','"],
			{ encoding: "utf8" },
		);
		if (ports.status !== 0) {
			throw new Error(".NET SerialPort is not available: " + ports.stderr);
		}
		return tool.script;
	});

	if (process.platform === "linux") {
		await step("Serial port permissions", async () => {
			const ports = await listPorts();
			if (ports.length === 0) {
				return "no board connected right now (not checked)";
			}
			for (const p of ports) {
				const hint = flasher.linuxPortHint(p.port);
				if (hint) {
					throw new Error(hint);
				}
			}
			return ports.map((p) => p.port).join(", ");
		});
	}
	return results;
}

async function runCheckInstall(context, manual) {
	const results = await checkInstall(context);
	output.appendLine("Installation check:");
	for (const r of results) {
		output.appendLine(`  [${r.ok ? "OK" : "FAIL"}] ${r.name}${r.detail ? " - " + r.detail : ""}`);
	}
	const failed = results.filter((r) => !r.ok);
	if (failed.length === 0) {
		await context.globalState.update("asm51.checkedVersion", context.extension.packageJSON.version);
		if (manual) {
			vscode.window.showInformationMessage("ASM51: installation check passed.");
		}
		return;
	}
	output.show(true);
	const choice = await vscode.window.showErrorMessage(
		`ASM51: installation check failed - ${failed.map((r) => r.name + ": " + r.detail).join(" | ")}`,
		"Show details",
	);
	if (choice) {
		output.show(true);
	}
}

function deactivate() {}

module.exports = { activate, deactivate };
