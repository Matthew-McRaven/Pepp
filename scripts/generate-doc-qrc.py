import glob, sys, os, pathlib
from xml.dom import minidom

# Given a directory in $1, recursively enumerate its files,
# and create a QRC with those contents
docdir = pathlib.Path(f"{sys.argv[1]}")
impl = minidom.getDOMImplementation()
doc = impl.createDocument(None, 'RCC', None)
rcc = doc.documentElement
rcc.appendChild(qresource:=doc.createElement('qresource'))
qresource.setAttribute('prefix', '/help/')


# Always use posix paths inside the QRC, since this is how Qt appears to work on Windows too.
# Alias the file based on its relative path to the docdir, for ease of access inside application.
for file in docdir.glob("**/*"):
  if file.is_dir(): continue
  el=doc.createElement('file')
  el.appendChild(doc.createTextNode(str(file.absolute().as_posix())))
  el.setAttribute('alias', str(file.relative_to(docdir).as_posix()))
  qresource.appendChild(el)

# Write out the QRC file to the filename specified in $2.
# Must only print first child node or the <?xml...> bit is included.
with open(f"{sys.argv[2]}", "w") as f: f.write(doc.childNodes[0].toprettyxml())
