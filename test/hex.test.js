const assert = require("assert");
const fs = require("fs");
const path = require("path");
const { parseHex, toFlashHex } = require("../lib/hex");

function rec(addr, data, type = 0) {
	const r = [data.length, addr >> 8, addr & 255, type, ...data];
	return ":" + [...r, (-r.reduce((a, b) => a + b, 0)) & 255].map((b) => b.toString(16).padStart(2, "0")).join("").toUpperCase();
}

// gap (org) case: bytes at 0x0000 and 0x0105 -> 512 bytes, gap = FF
let r = toFlashHex([rec(0, [1, 2, 3]), rec(0x105, [9, 8]), ":00000001FF"].join("\n"));
assert.strictEqual(r.size, 512);
let p = parseHex(r.hex);
assert.strictEqual(p.errors.length, 0);
assert.strictEqual(p.bytes.get(2), 3);
assert.strictEqual(p.bytes.get(3), 0xff);
assert.strictEqual(p.bytes.get(0x105), 9);
assert.strictEqual(p.bytes.get(0x1ff), 0xff);
assert.ok(r.hex.split("\n").filter((l) => l.startsWith(":10")).length === 32);

// too large
assert.throws(() => toFlashHex([rec(0xf800, [1]), ":00000001FF"].join("\n")), /beyond/);
// invalid checksum
assert.throws(() => toFlashHex(":0100000001FF\n:00000001FF\n"), /invalid/);
// empty
assert.throws(() => toFlashHex(":00000001FF\n"), /no data/);

const real = process.argv[2];
if (real) {
	const before = parseHex(fs.readFileSync(real, "utf8"));
	const after = toFlashHex(fs.readFileSync(real, "utf8"));
	const parsed = parseHex(after.hex);
	for (const [a, v] of before.bytes) assert.strictEqual(parsed.bytes.get(a), v, "byte " + a);
	console.log(`real file ok: ${before.bytes.size} bytes -> ${after.size} bytes (${after.pages} pages)`);
	if (process.argv[3]) fs.writeFileSync(process.argv[3], after.hex);
}
console.log("hex tests passed");
