#!/usr/bin/env python3
"""Fake ADuC832 serial loader on a pty, used to test the flasher.
Usage: fakeloader.py LINKPATH LOGFILE [silent]
Creates a pty, symlinks the slave to LINKPATH, logs validated packets to LOGFILE.
"""
import os, pty, sys, tty

link, logfile = sys.argv[1], sys.argv[2]
silent = len(sys.argv) > 3 and sys.argv[3] == 'silent'
master, slave = pty.openpty()
tty.setraw(slave)
if os.path.lexists(link):
    os.remove(link)
os.symlink(os.ttyname(slave), link)
log = open(logfile, 'w', buffering=1)
print('READY', flush=True)
flash = {}
buf = bytearray()


def rd(n):
    while len(buf) < n:
        buf.extend(os.read(master, 4096))
    out = bytes(buf[:n])
    del buf[:n]
    return out


def ack():
    os.write(master, b'\x06')


while True:
    b = rd(1)
    if b == b'\x21':
        rest = rd(3)
        if silent:
            continue
        if rest != b'\x5a\x00\xa6':
            log.write('BAD-INTERROGATE %s\n' % rest.hex())
            continue
        log.write('INTERROGATE\n')
        reply = bytearray(b'ADI 832') + bytearray(3) + b'V1.0' + bytearray(2) + b'\x01\x02'
        while len(reply) < 24:
            reply.append(0)
        reply.append((-sum(reply)) & 0xFF)
        os.write(master, bytes(reply))
    elif b == b'\x07':
        hdr = rd(1)
        if hdr == b'\x0e':
            n = rd(1)[0]
            body = rd(n + 1)
            ok = (n + sum(body)) & 0xFF == 0
            log.write('CMD %s len=%d data=%s chk=%s\n' % (chr(body[0]), n, body[1:-1].hex(), 'ok' if ok else 'BAD'))
            if ok:
                ack()
            else:
                os.write(master, b'\x07')
        elif hdr == b'\x51':
            data = rd(258)
            ok = sum(data) & 0xFF == 0
            flash[data[0]] = data[1:257]
            log.write('PAGE %d chk=%s\n' % (data[0], 'ok' if ok else 'BAD'))
            if ok:
                ack()
            else:
                os.write(master, b'\x07')
        else:
            log.write('BAD-HDR %s\n' % hdr.hex())
    else:
        log.write('JUNK %s\n' % b.hex())
    with open(logfile + '.img', 'wb') as f:
        for i in sorted(flash):
            f.seek(i * 256)
            f.write(flash[i])
