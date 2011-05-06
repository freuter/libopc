#!/usr/bin/python

import sys
import os
import getopt
import subprocess
import json
from xml.dom.minidom import parse

global target_mode

# XSD validator from http://www.ltg.ed.ac.uk/~ht/xsv-status.html.
xsv_path="c:\\Program Files\\XSV\\xsv.exe"
if not(os.path.exists(xsv_path)):
	print("Can not find \""+xsv_path+"\"");
	print("Please download the XSV validator from ")
	print("http://www.ltg.ed.ac.uk/~ht/xsv-status.html")
	print("and install it in "+os.path.split(xsv_path)[0])
	sys.exit(2)

schema_root="http://subversion.assembla.com/svn/IS29500/trunk/"
#schema_root="file:///C:/Projects/schema/trunk/"

validation_matrix={
	"image/png": None,
	"image/gif": None,
	"image/x-wmf": None,
	"image/x-emf": None,
	"image/jpeg": None,
	"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml": {"xsd": schema_root+"Part4/OfficeOpenXML-XMLSchema-Transitional/wml.xsd"},
	"application/vnd.openxmlformats-officedocument.extended-properties+xml": {"xsd": schema_root+"Part4/OfficeOpenXML-XMLSchema-Transitional/shared-documentPropertiesExtended.xsd"},
	"application/vnd.openxmlformats-package.core-properties+xml": {"xsd": schema_root+"Part2/OpenPackagingConventions-XMLSchema/opc-coreProperties.xsd"},
	"application/vnd.openxmlformats-officedocument.drawingml.diagramColors+xml": {"xsd": schema_root+"Part4/OfficeOpenXML-XMLSchema-Transitional/dml-diagram.xsd"},
	"application/vnd.openxmlformats-officedocument.drawingml.diagramData+xml": {"xsd": schema_root+"Part4/OfficeOpenXML-XMLSchema-Transitional/dml-diagram.xsd"},
	"application/vnd.ms-office.drawingml.diagramDrawing+xml": {"xsd": schema_root+"Part4/OfficeOpenXML-XMLSchema-Transitional/dml-diagram.xsd"},
	"application/vnd.openxmlformats-officedocument.wordprocessingml.fontTable+xml": {"xsd": schema_root+"Part4/OfficeOpenXML-XMLSchema-Transitional/wml.xsd"},
	"application/vnd.openxmlformats-officedocument.drawingml.diagramLayout+xml": {"xsd": schema_root+"Part4/OfficeOpenXML-XMLSchema-Transitional/dml-diagram.xsd"},
	"application/vnd.openxmlformats-officedocument.drawingml.diagramStyle+xml": {"xsd": schema_root+"Part4/OfficeOpenXML-XMLSchema-Transitional/dml-diagram.xsd"}
}

def validate(path):
	print("=== validate: "+path)
	tmp_dir=os.path.abspath(os.path.join("win32", target_mode))
	mce_extract_path=os.path.abspath(os.path.join("win32", target_mode, "mce_extract.exe"))
	if os.path.exists(mce_extract_path):
		_args = [ mce_extract_path ]
		_args.append(path)
		sys.stderr.write("invoking "+os.path.split(_args[0])[1]+" for "+path+"..."); sys.stderr.flush()
		p=subprocess.Popen(_args, stdout=subprocess.PIPE)
		parts=eval(p.communicate()[0])
		for part in parts:
			if part["type"] in validation_matrix:
				validation_hint=None
				validation_hint=validation_matrix[part["type"]]
				if not(validation_hint is None):
					print "validating "+part["name"]+" of type "+part["type"]
					validation_error=True
					validation_msg="Internal error invoking validator"
					_args = [ mce_extract_path ]
					_args.append(path)
					_args.append(part["name"])
#					_args.append("--understands"); _args.append("http://schemas.microsoft.com/office/word/2010/wordml")
					tmp_file=os.path.join(tmp_dir, "temp.xml")
					out_file=os.path.join(tmp_dir, "out.xml")
					if os.path.exists(tmp_file):
						os.remove(tmp_file)
					_args.append("--out"); _args.append(tmp_file)
					if True or not(os.path.exists(tmp_file)):
						sys.stderr.write("invoking "+os.path.split(_args[0])[1]+" for "+part["name"]+"..."); sys.stderr.flush()
						ret=subprocess.call(_args)
						if os.path.exists(tmp_file):
							if os.path.exists(out_file):
								os.remove(out_file)
							if not(os.path.exists(out_file)):
								_args = [ xsv_path ]
								_args.append("-o"); _args.append(os.path.relpath(out_file));
								_args.append(os.path.relpath(tmp_file).replace('\\', '/'))
								_args.append(validation_hint["xsd"])
								sys.stderr.write("invoking "+os.path.split(_args[0])[1]+" for "+part["name"]+"..."); sys.stderr.flush()
								returncode=subprocess.call(_args)
								sys.stderr.write("done\n"); sys.stderr.flush()
								if 0==returncode:
									result=parse(out_file)
									validation_error=not(result.documentElement.tagName == "xsv"
													and "true"==result.documentElement.getAttribute("instanceAssessed")
													and "0"==result.documentElement.getAttribute("instanceErrors")
													and "0"==result.documentElement.getAttribute("schemaErrors"))
								else:
									validation_msg="error executing "+os.path.split(_args[0])[1]
								if validation_error:
									validation_msg=result.toxml()
					if validation_error:
						print "**ERROR validating "+part["name"]
						print validation_msg
				else:
					print "ignoring "+part["name"]+" of type "+part["type"]
			else:
				print "skipping "+part["name"]+" of type "+part["type"]
			sys.stdout.flush()
	else:
		print "can not find mce_extract tool!"

def usage():
	print("usage:")
	print(" validate [--target=mode \"debug|release\"] sample.docx")


if __name__ == "__main__":
	sys.path.append(os.path.abspath("third_party"))
	target_mode=None
	input=[]
	try:
		opts, args = getopt.getopt(sys.argv[1:], "h", ["help", "target=", "generate=", "skip"])
	except getopt.GetoptError, err:
		# print help information and exit:
		print str(err) # will print something like "option -a not recognized"
		usage()
		sys.exit(2)
	for o, a in opts:
		if o in ("-h", "--help"):
			usage()
			sys.exit(0)
		elif o in ("--target"):
			target_mode=a
	if None==target_mode:
		print "please specify target mode, e.g. --target=debug"
		sys.exit(2)
	for f in args:
		validate(f)
