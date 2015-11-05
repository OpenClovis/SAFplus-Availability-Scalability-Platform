import string
import sys

if sys.version_info[0] >= 3:
 inp = input
else:
 inp = raw_input

distTemplate = string.Template("""origin: $product
Label: $product
Codename: $distro
Architectures: i386 amd64
Components: contrib
Description: Apt repository for Openclovis safplus
SignWith: $gpgkey
""")

def main(fil,prod,dist):
  f = open(fil,"w")

  gk = inp("Enter GPG key: ")
  gk = '"' + gk + '"'

  s = distTemplate.safe_substitute(product=prod,distro=dist,gpgkey=gk)
  f.write(s)
  f.close()
  return 0

if __name__ == "__main__":
  main(*sys.argv[1:])
