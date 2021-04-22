import sys
import os.path
print(sys.argv[:])
print("Building hardcoded OpenCL kernel in c string")

#with open(sys.argv[1]) as cpp_file:
#    cpp = cpp_file.read()

cl = "const char *KernelSource = \"\\n\" \\\n"
with open(sys.argv[2]) as cl_file:
    print("HHHH"+sys.argv[2])
    for line in cl_file.readlines():
        if(line.strip() != "\n"):
            cl += '\"'+line.strip() + '\\n\" \\\n'
cl += "\"\\n\";\n"

with open(os.path.splitext(sys.argv[1])[0] + ".h", "w") as cpp_file:
    cpp_file.write(cl)
