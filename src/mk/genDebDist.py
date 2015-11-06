import string
import sys
import os

if sys.version_info[0] >= 3:
 inp = input
else:
 inp = raw_input

distTemplate = string.Template("""origin: $product
Label: $product
Codename: $distro
Architectures: $Arch_type
Components: contrib
Description: Apt repository for Openclovis safplus
SignWith: $gpgkey
""")

def main(dir_path,prod,dist,arch):

  f = open(os.path.join(dir_path,"distributions"),"w")
  gk = inp("Enter GPG key: ")
  arch_type = arch.split("-")[0]
  if arch_type == "x86_64":
    arch_type = "amd64"
  else:
    arch_type == "i386"
  s = distTemplate.safe_substitute(product=prod, distro=dist, gpgkey=gk, Arch_type=arch_type)
  f.write(s)
  f.close()
  s = """
ask-passphrase
"""
  f = open(os.path.join(dir_path,"options"), "w")
  f.write(s)
  f.close()
  return 0

if __name__ == "__main__":
  main(*sys.argv[1:])
