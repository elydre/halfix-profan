import os, sys

profan_path = "../profanOS"
if sys.argv[1:]:
    profan_path = sys.argv[1]
if not os.path.exists(profan_path):
    print(f"path {profan_path} does not exist")
    exit(1)

CC      = "gcc"
LD      = "ld"

OUTPUT  = "halfix"

CFLAGS  = f"-ffreestanding -fno-exceptions -m32 -I ./include -D PROFAN -nostdinc -O3 -fno-stack-protector -fno-omit-frame-pointer -I {profan_path}/include/zlibs -I {profan_path}/include/addons"
LDFLAGS = f"-nostdlib -L {profan_path}/out/zlibs -T link.ld -lc -lm"

OBJDIR  = "build"
SRCDIRS  = ["src/cpu", "src/cpu/ops", "src/hardware", "src/host", "src"]

SRC = [os.path.join(d, e) for d in SRCDIRS for e in os.listdir(d) if e.endswith(".c")]

success = 0

def execute_command(cmd):
    global success
    print(cmd)
    rcode = os.system(cmd)
    if rcode == 0: return
    print(f"Command failed with exit code {rcode}")
    print(f"Successfully compiled {success} files / {len(SRC) + 2}")
    exit(rcode if rcode < 256 else 1)

def compile_file(src):
    global success
    obj = os.path.join(OBJDIR, src.replace(".c", ".o").split("/")[-1])
    cmd = f"{CC} -c {src} -o {obj} {CFLAGS}"
    execute_command(cmd)
    success += 1
    return obj

def link_files(entry, objs, output = OUTPUT):
    execute_command(f"{LD} {LDFLAGS} -o {output}.elf {entry} {' '.join(objs)} ")

def main():
    execute_command(f"mkdir -p {OBJDIR}")
    objs = [compile_file(src) for src in SRC]
    objs.append(compile_file("bordel.c"))

    entry = compile_file("entry.c")
    link_files(entry, objs)

if __name__ == "__main__":
    main()
