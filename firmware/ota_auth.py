# PlatformIO extra script for the esp32-ota env:
#  - hands espota the OTA password from include/secrets.h, so it lives in one (git-ignored) place;
#  - resolves firewatch.local itself, with retries. espota gives up on the first failed lookup,
#    and Windows' mDNS resolver misses now and then.
import os
import re
import socket
import time

Import("env")  # noqa: F821 (provided by PlatformIO)

path = os.path.join(env.subst("$PROJECT_DIR"), "include", "secrets.h")  # noqa: F821
try:
    with open(path, encoding="utf-8") as f:
        m = re.search(r'#define\s+OTA_PASSWORD\s+"([^"]*)"', f.read())
except OSError:
    m = None
if m:
    env.Append(UPLOADERFLAGS=["--auth=" + m.group(1)])  # noqa: F821
else:
    print("ota_auth.py: OTA_PASSWORD not found in include/secrets.h; OTA upload will be refused")


def resolve_upload_port(source, target, env):
    host = env.subst("$UPLOAD_PORT")
    if not host.endswith(".local"):
        return
    for attempt in range(15):  # a cold lookup can miss for several seconds
        try:
            ip = socket.gethostbyname(host)
            print("ota_auth.py: %s is %s" % (host, ip))
            env.Replace(UPLOAD_PORT=ip)
            return
        except OSError:
            time.sleep(1)
    print("ota_auth.py: couldn't resolve %s; pass --upload-port <ip>" % host)


env.AddPreAction("upload", resolve_upload_port)  # noqa: F821
