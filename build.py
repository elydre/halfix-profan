import os

CC      = "gcc"
LD      = "ld"

OUTPUT  = "halfix"

CFLAGS  = "-ffreestanding -fno-exceptions -m32 -I ./profan_zlib -I ./include -D PROFAN -O1"
LDFLAGS = "-T link.ld -Map=halfix.map"

OBJDIR  = "build"
SRCDIRS  = ["src/cpu", "src/cpu/ops", "src/hardware", "src/host", "src"]

#SRC = [e for d in SRCDIRS for e in os.listdir(d) if e.endswith(".c")]
SRC = [os.path.join(d, e) for d in SRCDIRS for e in os.listdir(d) if e.endswith(".c")]

success = 0

def execute_command(cmd):
    global success
    print(cmd)
    rcode = os.system(cmd)
    if rcode == 0: return
    print(f"Command failed with exit code {rcode}")
    print(f"Successfully compiled {success} files / {len(SRC)}")
    exit(rcode if rcode < 256 else 1)

def compile_file(src):
    global success
    obj = os.path.join(OBJDIR, src.replace(".c", ".o").split("/")[-1])
    cmd = f"{CC} -c {src} -o {obj} {CFLAGS}"
    execute_command(cmd)
    success += 1
    return obj

def link_files(entry, objs, output = OUTPUT):
    execute_command(f"{LD} {LDFLAGS} -o {output}.pe {entry} {' '.join(objs)} ")
    execute_command(f"objcopy -O binary {output}.pe {output}.bin")

def main():
    execute_command(f"mkdir -p {OBJDIR}")
    objs = [compile_file(src) for src in SRC]
    objs.append(compile_file("bordel.c"))

    entry = compile_file("entry.c")
    link_files(entry, objs)

    execute_command("rm *.pe")

if __name__ == "__main__":
    main()
