"use strict";
// Flasher support without any dependency on the vscode API (so it can be tested standalone).
const fs = require("fs");
const path = require("path");
const cp = require("child_process");

// Resolves the bundled flasher for this OS/CPU and makes it executable.
// Windows: PowerShell port (powershell.exe ships with Windows). Linux: prebuilt static lsd binary.
function prepare(extensionPath, storageDir) {
	if (process.platform === "win32") {
		const script = path.join(extensionPath, "tools", "win", "lsd-win.ps1");
		if (!fs.existsSync(script)) {
			throw new Error(`Flasher script missing: ${script}`);
		}
		return { kind: "ps", command: "powershell.exe", script };
	}
	if (process.platform !== "linux") {
		throw new Error("The bundled flasher supports Windows and Linux only. Set 'asm51.flash.toolPath' to your own tool.");
	}
	const dirName = { x64: "linux-x64", arm64: "linux-arm64" }[process.arch];
	if (!dirName) {
		throw new Error(`No bundled flasher for CPU type '${process.arch}'. Set 'asm51.flash.toolPath' to your own tool.`);
	}
	const src = path.join(extensionPath, "tools", dirName, "lsd");
	if (!fs.existsSync(src)) {
		throw new Error(`Flasher binary missing: ${src}`);
	}
	// The extension folder may be mounted noexec or lose exec bits, so run a copy from storage.
	const dir = path.join(storageDir, "lsd");
	const dest = path.join(dir, "lsd");
	fs.mkdirSync(dir, { recursive: true });
	const s = fs.statSync(src);
	let stale = true;
	try {
		stale = fs.statSync(dest).size !== s.size;
	} catch {}
	if (stale) {
		fs.copyFileSync(src, dest);
	}
	fs.chmodSync(dest, 0o755);
	return { kind: "lsd", command: dest };
}

function invocation(tool, { port, hex, run }) {
	if (tool.kind === "ps") {
		return {
			command: tool.command,
			args: [
				"-NoProfile",
				"-NonInteractive",
				"-ExecutionPolicy",
				"Bypass",
				"-File",
				tool.script,
				"-Port",
				port,
				...(run ? ["-Run"] : []),
				hex,
			],
		};
	}
	return { command: tool.command, args: [`-device=${port}`, ...(run ? ["-run"] : []), hex] };
}

// Runs the flasher with a watchdog. lsd always exits 0 and spins forever when no loader answers,
// so success is decided from its output and the connect/total timeouts are enforced here.
function runFlash(inv, { connectTimeoutMs = 12000, totalTimeoutMs = 120000, onData } = {}) {
	return new Promise((resolve) => {
		const child = cp.spawn(inv.command, inv.args, { windowsHide: true });
		let out = "";
		let connected = false;
		let reason = "";
		const kill = (why) => {
			reason = why;
			try {
				child.kill("SIGKILL");
			} catch {}
		};
		const connectTimer = setTimeout(() => !connected && kill("connect"), connectTimeoutMs);
		const totalTimer = setTimeout(() => kill("total"), totalTimeoutMs);
		const feed = (d) => {
			const text = d.toString("latin1");
			out += text;
			if (/Loader connected\./.test(out)) {
				connected = true;
			}
			if (onData) {
				onData(text);
			}
		};
		child.stdout.on("data", feed);
		child.stderr.on("data", feed);
		const finish = (code, spawnError) => {
			clearTimeout(connectTimer);
			clearTimeout(totalTimer);
			resolve({ ...classify({ code, out, connected, reason, spawnError }), out });
		};
		child.on("error", (e) => finish(null, e));
		child.on("close", (code) => finish(code));
	});
}

function classify({ code, out, connected, reason, spawnError }) {
	if (spawnError) {
		return { ok: false, message: `Could not start the flasher: ${spawnError.message}` };
	}
	if (reason === "connect") {
		return {
			ok: false,
			message:
				"The board did not answer. Put it in download (bootloader) mode, then try again: " +
				"hold the DOWNLOAD/BOOT button while pressing RESET, and check the selected serial port.",
		};
	}
	if (reason === "total") {
		return { ok: false, message: "Flashing timed out after 2 minutes. Reset the board and try again." };
	}
	const open = out.match(/Could not open[^\r\n]*/);
	if (open) {
		return { ok: false, message: open[0] + " Is another program (e.g. a terminal or Reads51) using the port?" };
	}
	const failed = out.match(/(Failed|Error):[^\r\n]*/);
	if (failed) {
		return { ok: false, message: failed[0].trim() };
	}
	if (code !== 0 || !connected || !/Total bytes sent: \d+/.test(out)) {
		return { ok: false, message: `The flasher stopped unexpectedly (exit code ${code}). See the ASM51 output.` };
	}
	return { ok: true, message: "Flash completed." };
}

// Linux serial ports are usually only accessible to the 'dialout' (or 'uucp') group.
function linuxPortHint(port) {
	if (process.platform !== "linux") {
		return undefined;
	}
	try {
		fs.accessSync(port, fs.constants.R_OK | fs.constants.W_OK);
		return undefined;
	} catch (e) {
		if (e.code === "ENOENT") {
			return `${port} does not exist. Check that the board is connected.`;
		}
		let group = "dialout";
		try {
			const groups = fs.readFileSync("/etc/group", "utf8");
			if (/^uucp:/m.test(groups) && !/^dialout:/m.test(groups)) {
				group = "uucp";
			}
		} catch {}
		return (
			`No permission to use ${port}. Run: sudo usermod -aG ${group} $USER ` +
			"then log out and back in (or reboot), and try again."
		);
	}
}

module.exports = { prepare, invocation, runFlash, classify, linuxPortHint };
