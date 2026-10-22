// Extension-level E2E: stubs the vscode API, runs real compile + flash against a fake loader.
// Usage: node e2e.js <source.asm> <port> <storageDir> [expectFail]
const Module = require("module");
const path = require("path");
const fs = require("fs");

const [src, port, storage, expectFail] = process.argv.slice(2);
const root = path.resolve(__dirname, "..");
const messages = [];
const commands = {};
const settings = {
	"flash.toolPath": "",
	"flash.args": ["${hex}"],
	"flash.port": port,
	"flash.run": true,
	assemblerPath: "",
	includePaths: [],
	compileOnSave: false,
};
const out = (s) => process.stdout.write(s);
const doc = { languageId: "asm51", uri: { scheme: "file", fsPath: path.resolve(src), toString: () => "file://" + src }, fileName: src, isDirty: false };
const vscode = {
	StatusBarAlignment: { Left: 1 },
	ProgressLocation: { Notification: 15 },
	DiagnosticSeverity: { Error: 0, Warning: 1 },
	Range: class {},
	Diagnostic: class {},
	Uri: { file: (p) => ({ fsPath: p, toString: () => "file://" + p }) },
	window: {
		createOutputChannel: () => ({ appendLine: (l) => out(l + "\n"), append: out, show() {}, clear() {}, dispose() {} }),
		createStatusBarItem: () => ({ show() {}, hide() {}, dispose() {} }),
		onDidChangeActiveTextEditor: () => ({ dispose() {} }),
		registerCustomEditorProvider: () => ({ dispose() {} }),
		setStatusBarMessage: () => {},
		get activeTextEditor() {
			return { document: doc };
		},
		showWarningMessage: async (m) => messages.push("WARN " + m),
		showInformationMessage: async (m) => messages.push("INFO " + m),
		showErrorMessage: async (m) => messages.push("ERR " + m),
		showQuickPick: async () => undefined,
		withProgress: async (_o, fn) => fn(),
	},
	languages: { createDiagnosticCollection: () => ({ clear() {}, set() {}, delete() {}, dispose() {} }) },
	workspace: {
		workspaceFolders: [],
		getConfiguration: () => ({ get: (k) => settings[k] }),
		onDidChangeConfiguration: () => ({ dispose() {} }),
		onDidSaveTextDocument: () => ({ dispose() {} }),
		onDidCloseTextDocument: () => ({ dispose() {} }),
		onDidChangeTextDocument: () => ({ dispose() {} }),
	},
	commands: {
		registerCommand: (id, fn) => ((commands[id] = fn), { dispose() {} }),
		executeCommand: async () => {},
	},
};
const origLoad = Module._load;
Module._load = function (request, ...rest) {
	return request === "vscode" ? vscode : origLoad.call(this, request, ...rest);
};

const state = {};
const context = {
	subscriptions: [],
	extensionPath: root,
	globalStorageUri: { fsPath: path.resolve(storage) },
	globalState: { get: (k) => state[k], update: async (k, v) => (state[k] = v) },
	extension: { packageJSON: { version: "9.9.9" } },
};

(async () => {
	require(path.join(root, "extension.js")).activate(context);
	await new Promise((r) => setTimeout(r, 6000)); // let the automatic install check finish
	console.log("checked version stored:", state["asm51.checkedVersion"]);
	messages.length = 0;
	await commands["asm51.flash"]();
	console.log("--- messages:", JSON.stringify(messages));
	const ok = messages.some((m) => m.startsWith("INFO ASM51: flash completed"));
	if (expectFail) {
		process.exit(!ok && messages.some((m) => m.startsWith("ERR")) ? 0 : 1);
	}
	process.exit(ok ? 0 : 1);
})().catch((e) => {
	console.error("E2E crashed", e);
	process.exit(2);
});
