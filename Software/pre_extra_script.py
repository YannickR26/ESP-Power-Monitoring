import os
import gzip
import shutil

# Create a .gz file
def make_gzip(src_path: str, dst_path: str):
    with open(src_path, 'rb') as src, gzip.open(dst_path, 'wb') as dst:
        for chunk in iter(lambda: src.read(4096), b""):
            dst.write(chunk)


# Get a content type
def get_content_type(file: str):
    if file.endswith(".htm"):
        return "text/html"
    elif file.endswith(".html"):
        return "text/html"
    elif file.endswith(".json"):
        return "application/json"
    elif file.endswith(".css"):
        return "text/css"
    elif file.endswith(".js"):
        return "application/javascript"
    elif file.endswith(".png"):
        return "image/png"
    elif file.endswith(".gif"):
        return "image/gif"
    elif file.endswith(".jpg"):
        return "image/jpeg"
    elif file.endswith(".ico"):
        return "image/x-icon"
    elif file.endswith(".xml"):
        return "text/xml"
    elif file.endswith(".pdf"):
        return "application/x-pdf"
    elif file.endswith(".zip"):
        return "application/x-zip"
    elif file.endswith(".gz"):
        return "application/x-gzip"


# If tar dir exist, we delete it
if os.path.isdir("data/tar"):
    shutil.rmtree("data/tar")

# If dist dir exist, we delete it
if os.path.isdir("data/dist"):
    shutil.rmtree("data/dist")

# Change directory
os.chdir("data")
files = os.listdir()

# Create Dir
os.mkdir("tar")

print("Moving to prod env...")

# Replace dev_env by prod_env on all .html files
for file in files:
    if file.endswith(".html"):
        data = ""
        with open(file, "r") as f:
            data = f.read()

        data = data.replace('<script src="dev_env.js"></script>', '<script src="prod_env.js"></script>')

        with open(file, "w") as f:
            f.write(data)
            f.flush()

print("Create gz files...")

# For all file in dir, we create a .gz file
for file in files:
    make_gzip(file, f"tar/{file}.gz")

os.chdir("../")

# Create Dir
os.mkdir("data/dist")

# Create .h file for every .gz files
for file in files:
    defineName = file.replace(".", "_")
    defineName = defineName.replace("-", "_")
    content_type = get_content_type(file)
    print(f"Create file: data/dist/{file}.h")
    # Read .gz file as binary
    with open(f"data/tar/{file}.gz", "rb") as source:
        datas = source.read()
        # Create .h file
        with open(f"data/dist/{file}.h", "w") as dest:
            # Add header
            dest.write(f"#define {defineName}_len {len(datas)}\n")
            dest.write(f"#define {defineName}_content_type \"{content_type}\"\n")
            dest.write(f"const char {defineName}[] PROGMEM = ")
            dest.write("{\n")

            # Add data
            for i, data in enumerate(datas):
                dest.write(f"0x{data:02X}")
                if i < (len(datas)-1):
                    dest.write(",")

            dest.write("\n};")


# Create .h file with all files
with open(f"src/web_autogen.h", "w") as dest:
    dest.write("#pragma once\n\n")
    dest.write("#include <arduino.h>\n")
    dest.write("#include <WebServer.h>\n\n")
    for file in files:
        dest.write(f'#include "../data/dist/{file}.h"\n')
    dest.write("\nbool checkAndSendFile(String path, WebServer &server)\n")
    dest.write("{\n")
    for file in files:
        defineName = file.replace(".", "_")
        defineName = defineName.replace("-", "_")
        dest.write(f'\tif (path.endsWith("{file}"))\n')
        dest.write("\t{\n")
        dest.write('\t\tserver.sendHeader("Content-Encoding", "gzip");\n')
        dest.write(f'\t\tserver.send_P(200, {defineName}_content_type, {defineName}, {defineName}_len);\n')
        dest.write("\t\treturn true;\n")
        dest.write("\t}\n")
    dest.write("\n\treturn false;\n")
    dest.write("}\n")


os.chdir("data")

print("Moving to dev env...")

# Replace dev_env by prod_env on all .html files
for file in files:
    if file.endswith(".html"):
        data = ""
        with open(file, "r") as f:
            data = f.read()

        data = data.replace('<script src="prod_env.js"></script>', '<script src="dev_env.js"></script>')

        with open(file, "w") as f:
            f.write(data)
            f.flush()

os.chdir("../")

print("cleaning folder...")

# If tar dir exist, we delete it
if os.path.isdir("data/tar"):
    shutil.rmtree("data/tar")