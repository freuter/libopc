#!/usr/bin/python

import sys
import os
import copy
import xml.etree.ElementTree as etree
import uuid
import getopt

EXT_MAPPING = { ".c": "c", ".cc": "cpp", ".cpp": "cpp", ".h": "h", ".s": "s"}

def parseError(str):
	print("*** error: \""+str+"\"\n");

def platformCompare(a, b):
	return cmp(a.count('*'), b.count('*'))


def platformSubseteqTest(a, b):
	a3=a.split("-")
	b3=b.split("-")
	ret=len(a3)==len(b3)
	i=0
	while i<len(a3) and i<len(b3):		
		ret=ret and (a3[i]==b3[i] or b3[i]=="*")
		i=i+1
#	sys.stderr.write("platformSubseteqTest("+a+", "+b+")="+str(ret))
	return ret

def parseFile(node, ctx, list):
	path=node.attrib["path"]
	abs_path=os.path.join(ctx["base"], ctx["root"], path)
#	sys.stderr.write "path="+abs_path
	filterMatch=True
	if "platform" in node.attrib:
		filter=node.attrib["platform"]
		filterMatch=platformSubseteqTest(ctx["platform"], filter)
		
	if filterMatch:
		file_ext=os.path.splitext(path)[1]
		if file_ext in EXT_MAPPING:
			ext=EXT_MAPPING[file_ext]
			if os.path.exists(abs_path):
				install=None
				if "install" in node.attrib and node.attrib["install"]=="yes":				
					install=os.path.dirname(os.path.relpath(abs_path, ctx["root"]))
##					sys.stderr.write install

				list.append({ "path": abs_path, "ext": ext, "install": install })
			else:
				parseError("file "+abs_path+" does not exist!")
		else:
			parseError("extension "+file_ext+" is unknown")

def parseFiles(target, node, ctx):
	for child in list(node):
		if child.tag=="file":
			parseFile(child, ctx, target)
		else:
			parseError("unhandled element: "+child.tag)

def setAttr(dict, node, tag):
	if tag in node.attrib:
		dict[tag]=node.attrib[tag]		

def updateCtx(ctx, node):
	if "root" in node.attrib:
		root=ctx["root"]
		ctx=copy.copy(ctx)	
		v=node.attrib["root"].format(platform=ctx["platform"], config=ctx["config"])
		ctx["root"]=os.path.join(root, v)
	return ctx

def parseDefines(ctx, node, dict):
	value=None
	if "value" in node.attrib:
		value=node.attrib["value"]
	if "name" in node.attrib:
		name=node.attrib["name"]
		dict[name]=value	
	else:
       		parseError("define syntax invalid ("+nv+")");

def parseSettings(conf, node, ctx, lib):
	if node.tag=="settings":
		tag="defines"
	elif node.tag=="export":
		tag="exports"
	else:
       		parseError("unknown setting: "+node.tag);
	for child in list(node):
		if child.tag=="define":
			parseDefines(ctx, child, lib[tag])	
                else:
			parseError("unhanded element: "+child.tag)

def parseDeps(node):	
	if "dep" in node.attrib:
		dep=node.attrib["dep"].split()
		return dep
	else:
		return []



def parseLibrary(conf, node, ctx, seq_type):
	lib_name=node.attrib["name"]
	lib_uuid=uuid.uuid5(uuid.NAMESPACE_URL, "urn:lib:"+lib_name)
	lib={"name": lib_name, "uuid": lib_uuid, "source": { "files": []}, "header": { "files": [], "target": "", "includes": [] }, "defines": {}, "exports": {}, "deps": parseDeps(node), "mode": "c90", "align": "", "external": False }	
	ctx=updateCtx(ctx, node)
	setAttr(lib, node, "mode")
	setAttr(lib, node, "align")
	if not(lib_name in ctx["externals"]) or not("external" in ctx["externals"][lib_name]) or not(ctx["externals"][lib_name]["external"]):
		for child in list(node):
			if child.tag=="source":
				parseFiles(lib["source"]["files"], list(child), updateCtx(ctx, child))
			elif child.tag=="header":
				setAttr(lib["header"], child, "target")
				child_ctx=updateCtx(ctx, child)
				lib["header"]["includes"].append(child_ctx["root"])
				parseFiles(lib["header"]["files"], list(child), child_ctx)
			elif child.tag=="settings":
				parseSettings(conf, child, ctx, lib)
			elif child.tag=="export":
				parseSettings(conf, child, ctx, lib)
			else:
				parseError("unhanded element: "+child.tag)
	else:
		sys.stderr.write("using external "+lib_name)
		lib["external"]=True
	conf[seq_type].append(lib)

def isExcluded(conf, ctx, lib):
#	sys.stderr.write conf["platforms"]
	if ctx["platform"] in conf["platforms"]:
		platform=conf["platforms"][ctx["platform"]]
		return lib in platform["exclude"]
	else:
		return False

def parsePlatform(conf, node, ctx):
	if "name" in node.attrib:
		name=node.attrib["name"]
		platform={ "exclude": {}, "libs": [] }
		setAttr(platform, node, "cc")
		setAttr(platform, node, "ar")
		setAttr(platform, node, "ld")
		setAttr(platform, node, "cflags")
		setAttr(platform, node, "cflags_c")
		setAttr(platform, node, "cflags_cpp")
		setAttr(platform, node, "cppflags")
		setAttr(platform, node, "host")
		if "exclude" in node.attrib:
			for prj in node.attrib["exclude"].split(" "):
				platform["exclude"][prj]=None
#			sys.stderr.write platform["exclude"]
		if "libs" in node.attrib:
			platform["libs"].extend(node.attrib["libs"].split(" "))
		conf["platforms"][name]=platform
	else:
		parseError("no name given for platform")
		

def parseTargets(conf, node, ctx):
	for child in list(node):
		if child.tag=="include":
			path=os.path.abspath(os.path.join(ctx["root"], child.attrib["path"]))
			new_ctx=copy.copy(ctx)		
			new_ctx["root"]=os.path.dirname(path)
			parseConfiguration(conf, path, new_ctx)
		elif child.tag=="library":
			parseLibrary(conf, child, ctx, "libraries")
		elif child.tag=="tool":
			parseLibrary(conf, child, ctx, "tools")
		elif child.tag=="platform":
			parsePlatform(conf, child, ctx)
		else:
			parseError("unhanded element: "+child.tag)

def parseConfiguration(conf, filename, ctx):
	sys.stderr.write("parsing "+filename+"\n")
	tree=etree.parse(filename)
	if tree is not None:
		parseTargets(conf, tree.getroot(), ctx)
	else:
		parseError("failed to open file \""+filename+"\"")

def depClosure(conf, deps, ignoreExternal):
	ret=copy.copy(deps)
	change=True
	while change:
		change=False			
		for lib in conf["libraries"]:
			if lib["name"] in ret:
				lib_deps=lib["deps"]
				for lib_dep in lib_deps:
					if not lib_dep in ret:
						ret.append(lib_dep)
						change=True
#	sys.stderr.write str(deps)+"==>"+str(ret)
	if ignoreExternal:
		ret_clean=[]
		for lib in conf["libraries"]:
			for dep in ret:
				if lib["name"]==dep and not(lib["external"]):
					ret_clean.append(dep)
		return ret_clean
	else:
		return ret

def gatherIncludeDirs(conf, ctx, deps):
	ret=[]
	aux={}
	for lib in conf["libraries"]:
		if lib["name"] in deps:
			for file in lib["header"]["files"]:
				if file["install"] is not None:
					dir=os.path.dirname(file["path"])
					dir=os.path.abspath(dir.rstrip(file["install"]))
					dir=os.path.relpath(dir, ctx["base"])
					if dir not in aux:
						aux[dir]=None
						ret.append(dir)
#	sys.stderr.write ret
	return ret

def gatherSources(conf, ctx, lib):
	sources=[]
	for file in lib["source"]["files"]:
		rel_source=os.path.relpath(file["path"], ctx["base"])
		sources.append(rel_source)
	return sources

def generateOBJS(conf, ctx, lib, out, obj_dir, cppflags):
	objs=[]
	for file in lib["source"]["files"]:
		rel_source=os.path.relpath(file["path"], ctx["base"])
		rel_obj=os.path.join(obj_dir, os.path.splitext(rel_source)[0]+".o")
		out_dir=os.path.dirname(rel_obj)
		out.write(rel_obj+": "+rel_source+"\n")
		out.write("\t@mkdir -p "+out_dir+"\n")
		specific=""
		if file["ext"]=="c":
			specific ="$(CFLAGS_C) "
		elif file["ext"]=="cpp":
			specific ="$(CFLAGS_CPP) "
		out.write("\t$(CC) -c $(CFLAGS) "+ specific +"$(CPPFLAGS)"+cppflags+" "+ rel_source+" -o "+rel_obj+"\n")
		objs.append(rel_obj)
	return objs

def generateDEPS(conf, ctx, lib, out, obj_dir, cppflags, filename):
	for file in lib["source"]["files"]:
		rel_source=os.path.relpath(file["path"], ctx["base"])
		specific=""
		if file["ext"]=="c":
			specific ="$(CFLAGS_C) "
		elif file["ext"]=="cpp":
			specific ="$(CFLAGS_CPP) "
		out.write("\tmakedepend -a -f"+filename+" $(CPPFLAGS)"+cppflags+" "+ rel_source+"\n")

def generateCPPFLAGS(conf, ctx, lib, build_dir):
	cppflags=""
	for define in lib["defines"]:
		if lib["defines"][define] is None:
			cppflags=cppflags+" -D"+define
		else:
			cppflags=cppflags+" -D"+define+"="+lib["defines"][define]
	for dir in lib["header"]["includes"]:
		abs_dir=os.path.abspath(dir)
		rel_dir=os.path.relpath(abs_dir, os.path.join(ctx["base"], build_dir))
#		sys.stderr.write("abs_dir="+str(abs_dir)+" build_dir="+os.path.join(ctx["base"], build_dir)+" rel_dir="+str(rel_dir))
		cppflags=cppflags+" -I"+rel_dir
	dep_closure=depClosure(conf, lib["deps"], False)
	dep_dirs=gatherIncludeDirs(conf, ctx, dep_closure)
	for dir in dep_dirs:
		abs_dir=os.path.abspath(dir)
		rel_dir=os.path.relpath(abs_dir, os.path.join(ctx["base"], build_dir))
#		sys.stderr.write("abs_dir="+str(abs_dir)+" build_dir="+os.path.join(ctx["base"], build_dir)+" rel_dir="+str(rel_dir))
		cppflags=cppflags+" -I"+rel_dir
	for dep_lib in conf["libraries"]:
		if dep_lib["name"] in ctx["externals"]:
			ext_lib=ctx["externals"][dep_lib["name"]]
			if "external" in ext_lib and ext_lib["external"]:						
				if "cppflags" in ext_lib:
					cppflags=cppflags+" "+ext_lib["cppflags"]

	return cppflags


def generateVCXPROJ(conf, ctx, lib, type):
	filename=os.path.abspath("win32\\"+lib["name"]+"\\"+lib["name"]+".vcxproj")
	filepath=os.path.dirname(filename)
	prj_uuid=lib["uuid"]
	sys.stderr.write("generating "+filename+"\n")
	try:
		os.makedirs(os.path.dirname(filename))
	except OSError:
		pass

	out=open(filename, "w")
	out.write("<Project DefaultTargets=\"Build\" ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n");
        out.write("<ItemGroup Label=\"ProjectConfigurations\">\n");
        out.write(" <ProjectConfiguration Include=\"Debug|Win32\">\n");
        out.write("   <Configuration>Debug</Configuration>\n");
        out.write("   <Platform>Win32</Platform>\n");
        out.write(" </ProjectConfiguration>\n");
        out.write("<ProjectConfiguration Include=\"Release|Win32\">\n");
        out.write("   <Configuration>Release</Configuration>\n");
        out.write("   <Platform>Win32</Platform>\n");
        out.write("</ProjectConfiguration>\n");
	out.write("</ItemGroup>\n");
        out.write("<PropertyGroup Label=\"Globals\">\n");
        out.write("    <ProjectGuid>{"+str(prj_uuid)+"}</ProjectGuid>\n");
        out.write("    <Keyword>Win32Proj</Keyword>\n");
        out.write("    <RootNamespace>"+lib["name"]+"</RootNamespace>\n");
        out.write("</PropertyGroup>\n");
        out.write("<Import Project=\"$(VCTargetsPath)\Microsoft.Cpp.default.props\" />\n");
        out.write("  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\" Label=\"Configuration\">\n");
        out.write("    <ConfigurationType>"+type+"</ConfigurationType>\n");
        out.write("    <UseDebugLibraries>true</UseDebugLibraries>\n");
        out.write("    <CharacterSet>Unicode</CharacterSet>\n");
        out.write("  </PropertyGroup>\n");
        out.write("  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\" Label=\"Configuration\">\n");
        out.write("    <ConfigurationType>StaticLibrary</ConfigurationType>\n");
        out.write("    <UseDebugLibraries>false</UseDebugLibraries>\n");
        out.write("    <WholeProgramOptimization>true</WholeProgramOptimization>\n");
        out.write("    <CharacterSet>Unicode</CharacterSet>\n");
        out.write("  </PropertyGroup>\n");
        out.write("<Import Project=\"$(VCTargetsPath)\Microsoft.Cpp.props\" />\n");
        out.write("<ImportGroup Label=\"ExtensionSettings\" />\n");
        out.write("<ImportGroup Label=\"PropertySheets\" />\n");
        out.write("<PropertyGroup Label=\"UserMacros\" />\n");
        out.write("<PropertyGroup />\n");
        out.write("<ItemDefinitionGroup>\n");
	out.write("<ClCompile>\n");
	dep_closure=depClosure(conf, lib["deps"], False)
	cpp_defs="WIN32;"
	for define in lib["defines"]:
		if lib["defines"][define] is None:
			cpp_defs=cpp_defs+define+";"
		else:
			cpp_defs=cpp_defs+define+"="+lib["defines"][define]+";"
	for dep in dep_closure:
		for dep_lib in conf["libraries"]:
			if dep_lib["name"]==dep and not isExcluded(conf, ctx, dep):
				for define in dep_lib["exports"]:
					if dep_lib["exports"][define] is None:
						cpp_defs=cpp_defs+define+";"
					else:
						cpp_defs=cpp_defs+define+"="+dep_lib["defines"][define]+";"
	cpp_defs=cpp_defs+"%(PreprocessorDefinitions)"
	out.write("<PreprocessorDefinitions>"+cpp_defs+"</PreprocessorDefinitions>\n" )
	AdditionalIncludeDirectories=""
	for dir in lib["header"]["includes"]:
		AdditionalIncludeDirectories=AdditionalIncludeDirectories+os.path.relpath(os.path.abspath(dir), filepath)+";"	
	dep_dirs=gatherIncludeDirs(conf, ctx, dep_closure)
	for dir in dep_dirs:
		abs_path=os.path.relpath(os.path.abspath(dir), filepath)
		AdditionalIncludeDirectories=AdditionalIncludeDirectories+abs_path+";"
	AdditionalIncludeDirectories=AdditionalIncludeDirectories+"%(AdditionalIncludeDirectories)"
	out.write("<AdditionalIncludeDirectories>"+AdditionalIncludeDirectories+"</AdditionalIncludeDirectories>\n")
	if "c99"==lib["mode"]:
		out.write("<CompileAs>CompileAsCpp</CompileAs>\n");
	if ""!=lib["align"]:
		out.write("<StructMemberAlignment>"+lib["align"]+"Byte</StructMemberAlignment>\n");
	out.write("</ClCompile>\n");
	if "Application"==type:
		out.write("<Link>\n");
		out.write("<AdditionalDependencies>")
		if ctx["platform"] in conf["platforms"]:
			ext_libs=conf["platforms"][ctx["platform"]]["libs"]
		else:
			ext_libs={}
#		sys.stderr.write ext_libs
		for ext_lib in ext_libs:
#			sys.stderr.write ext_lib
			out.write(ext_lib+";")
		out.write("%(AdditionalDependencies)</AdditionalDependencies> \n")
		out.write("</Link>\n");
        out.write("</ItemDefinitionGroup>\n");


	out.write("<ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">\n")
	out.write("<ClCompile>\n");
	out.write("<WarningLevel>Level3</WarningLevel>\n")
	out.write("<Optimization>Disabled</Optimization>\n")
	out.write("</ClCompile>\n");
	if "Application"==type:
		out.write("<Link>\n");
		out.write("<GenerateDebugInformation>true</GenerateDebugInformation>\n")
		out.write("</Link>\n");	
        out.write("</ItemDefinitionGroup>\n");

	out.write("<ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">\n")
	out.write("<ClCompile>\n")
	out.write("<Optimization>MaxSpeed</Optimization>\n")
	out.write("<FunctionLevelLinking>true</FunctionLevelLinking> \n")
	out.write("<IntrinsicFunctions>true</IntrinsicFunctions> \n")
	out.write("</ClCompile>\n");
	if "Application"==type:
		out.write("<Link>\n");
		out.write("<GenerateDebugInformation>false</GenerateDebugInformation>\n")
		out.write("<EnableCOMDATFolding>true</EnableCOMDATFolding>\n")
		out.write("<OptimizeReferences>true</OptimizeReferences>\n")
		out.write("</Link>\n");	
        out.write("</ItemDefinitionGroup>\n");

	out.write("<ItemGroup>\n");
        for file in lib["header"]["files"]:
		if "h"==file["ext"]:
			out.write("  <ClInclude Include=\""+os.path.relpath(file["path"], filepath)+"\"/>\n");
        out.write("</ItemGroup>\n");
        out.write("<ItemGroup>\n");
        for file in lib["source"]["files"]:
		if "cpp"==file["ext"] or "c"==file["ext"] or "s"==file["ext"]:
			out.write("  <ClCompile Include=\""+os.path.relpath(file["path"], filepath)+"\">\n");
			out.write("  </ClCompile>\n");
        out.write("</ItemGroup>\n");
        out.write("<ItemGroup>\n");
	for dep in dep_closure:
		for dep_lib in conf["libraries"]:
			if dep_lib["name"]==dep and not isExcluded(conf, ctx, dep_lib["name"]):
				out.write("<ProjectReference Include=\"../"+dep_lib["name"]+"/"+dep_lib["name"]+".vcxproj\">\n")
				out.write("<Project>{"+str(dep_lib["uuid"])+"}</Project>\n")
				out.write("</ProjectReference>\n")
        out.write("</ItemGroup>\n")
        out.write("<Import Project=\"$(VCTargetsPath)\Microsoft.Cpp.targets\" />\n");
        out.write("<ImportGroup Label=\"ExtensionTargets\" />\n");
        out.write("</Project>\n");
	out.close()

def writeProject(out, solution_uuid, lib):
	out.write("Project(\"{"+str(solution_uuid)+"}\") = \""+lib["name"]+"\", \""+lib["name"]+"\\"+lib["name"]+".vcxproj\", \"{"+str(lib["uuid"])+"}\"\n")
	out.write("EndProject\n")

def generateConfiguration(ctx, includes, platform):
	conf={ "libraries": [], "tools": [], "platforms": {} }
	ctx["platform"]=platform
	for include in includes:
		parseConfiguration(conf, include, ctx)
	if ctx["platform"] not in conf["platforms"]:	
		add=[]
		for platform in conf["platforms"]:
			if platformSubseteqTest(ctx["platform"], platform):
				add.append(platform)
		add.sort(platformCompare)
		if len(add)>0:
			conf["platforms"][ctx["platform"]]=conf["platforms"][add[0]]
	return conf

def generateWin32(ctx, source):
	platform=ctx["platforms"][0]
	conf=generateConfiguration(ctx, [source], platform)
#	sys.stderr.write conf
        sys.stderr.write("generate win32 project")
	for lib in conf["libraries"]:
		if not isExcluded(conf, ctx, lib["name"]):
			generateVCXPROJ(conf, ctx, lib, "StaticLibrary")
	for tool in conf["tools"]:
		if not isExcluded(conf, ctx, tool["name"]):
			generateVCXPROJ(conf, ctx, tool, "Application")
	solution_file=os.path.abspath("win32\\solution.sln")
	solution_uuid=uuid.uuid5(uuid.NAMESPACE_URL, solution_file)
	out=open(solution_file, "w")
	out.write("\n")
	out.write("Microsoft Visual Studio Solution File, Format Version 11.00\n")
	for lib in conf["libraries"]:
		if not isExcluded(conf, ctx, lib["name"]):
			writeProject(out, solution_uuid, lib)
	for tool in conf["tools"]:
		if not isExcluded(conf, ctx, tool["name"]):
			writeProject(out, solution_uuid, tool)
	out.write("\n")
	out.close()

def writeSourceRules(conf, ctx, out, lib, build_dir):
	dirs={}
	sources=gatherSources(conf, ctx, lib)
	out.write("objs_"+lib["name"]+"=\\\n")
	for source in sources:
		out.write("     $(BUILD_DIR)/"+os.path.splitext(source)[0]+".o\\\n")
		dirs[os.path.dirname(source)]=None
	out.write("     \n")
	out.write(lib["name"]+"_includes="+generateCPPFLAGS(conf, ctx, lib, build_dir)+"\n")
	out.write("\n")
	includes="$("+lib["name"]+"_includes)"
	for dir in dirs:
		for ext in EXT_MAPPING:
			if EXT_MAPPING[ext]=='c':
				out.write("$(BUILD_DIR)/"+dir+"/%.o: $(SRC_DIR)/"+dir+"/%"+ext+"\n")
				out.write("\t@mkdir -p $(dir $@)\n")
				out.write("\t$(CC) $(CFLAGS) $(CFLAGS_C) $(CPPFLAGS) "+includes+" -c \"$<\" -o \"$@\"\n")
				out.write("\n")
			elif EXT_MAPPING[ext]=='cpp':
				out.write("$(BUILD_DIR)/"+dir+"/%.o: $(SRC_DIR)/"+dir+"/%"+ext+"\n")
				out.write("\t@mkdir -p $(dir $@)\n")
				out.write("\t$(CC) $(CFLAGS) $(CFLAGS_CPP) $(CPPFLAGS) "+includes+" -c \"$<\" -o \"$@\"\n")
				out.write("\n")

def writeMakefileFlags(conf, ctx, out):
	platform=ctx["platform"]
	out.write("CC="+conf["platforms"][platform]["cc"]+"\n");
	out.write("AR="+conf["platforms"][platform]["ar"]+"\n");
	out.write("CFLAGS="+conf["platforms"][platform]["cflags"]+"\n")
	out.write("CFLAGS_C="+conf["platforms"][platform]["cflags_c"]+"\n")
	out.write("CFLAGS_CPP="+conf["platforms"][platform]["cflags_cpp"]+"\n")
	out.write("CPPFLAGS="+conf["platforms"][platform]["cppflags"]+"\n");

def generateLibraryMakefile(conf, ctx, lib, filename, build_dir, src_dir):
	sys.stderr.write("generating "+filename+"\n")
	if not os.path.exists(os.path.dirname(filename)):
    		os.makedirs(os.path.dirname(filename))
	out=open(filename, "w")
	out.write("# Generated.\n")
	# compiler flags
	writeMakefileFlags(conf, ctx, out)
	out.write("BUILD_DIR=.\n")
	out.write("SRC_DIR="+src_dir+"\n")
	out.write("\n")
	out.write(".phony: all clean\n")
	out.write("\n")
	out.write("# library "+lib["name"]+"\n")
	out.write("all: $(BUILD_DIR)/lib"+lib["name"]+".a\n\n")
	writeSourceRules(conf, ctx, out, lib, build_dir)
	out.write("$(BUILD_DIR)/lib"+lib["name"]+".a: $(objs_"+lib["name"]+")\n")
	out.write("\t$(AR) -rcs $(BUILD_DIR)/lib"+lib["name"]+".a $(objs_"+lib["name"]+")\n")
	out.write("\n")
	out.write("clean:\n")
	out.write("\trm -f $(BUILD_DIR)/lib"+lib["name"]+".a $(objs_"+lib["name"]+")\n")
	out.close()

def generateToolMakefile(conf, ctx, tool, filename, build_dir, src_dir):
	sys.stderr.write("generating "+filename+"\n")
	if not os.path.exists(os.path.dirname(filename)):
    		os.makedirs(os.path.dirname(filename))
	out=open(filename, "w")
	out.write("# Generated.\n")
	# compiler flags
	writeMakefileFlags(conf, ctx, out)
	out.write("BUILD_DIR=.\n")
	out.write("SRC_DIR="+src_dir+"\n")
	out.write("\n")
	out.write(".phony: all clean\n")
	out.write("\n")
	out.write("# tool "+tool["name"]+"\n")
	out.write("all: $(BUILD_DIR)/"+ tool["name"]+"\n\n")
	writeSourceRules(conf, ctx, out, tool, build_dir)
	out.write(tool["name"]+"_ld=")
	deps=depClosure(conf, tool["deps"], True)
	for dep in deps:
		out.write(" $(BUILD_DIR)/lib"+ dep +".a")
	out.write("\n")
	ext_ldflags=""
	for dep_lib in conf["libraries"]:
		if dep_lib["name"] in ctx["externals"]:
			ext_lib=ctx["externals"][dep_lib["name"]]
			if "external" in ext_lib and ext_lib["external"]:						
				if "ldflags" in ext_lib:
					ext_ldflags=ext_ldflags+" "+ext_lib["ldflags"]

	out.write("$(BUILD_DIR)/"+tool["name"]+": $(objs_"+tool["name"]+") $("+tool["name"]+"_ld)\n")			
	out.write("\t$(CC) $(CFLAGS) $(CPPFLAGS) -o $(BUILD_DIR)/"+tool["name"]+" $(objs_"+tool["name"]+") $("+tool["name"]+"_ld)"+ ext_ldflags+"\n")
	out.write("\n")
	out.write("clean:\n")
	out.write("\trm -f $(BUILD_DIR)/"+tool["name"]+" $(objs_"+tool["name"]+")\n")


def generateMakefile(conf, ctx):
	obj_dir=os.path.join("build", ctx["platform"])
	filename=os.path.join(obj_dir, "Makefile")
	sys.stderr.write("generating "+filename+"\n")
	if not os.path.exists(os.path.dirname(filename)):
    		os.makedirs(os.path.dirname(filename))
	out=open(filename, "w")
	out.write("# Generated.\n")

	# generate phony targets
	out.write(".phony: all clean tools dep")
	for lib in conf["libraries"]:
		if not lib["external"]:
			out.write(" "+lib["name"])
	for tool in conf["tools"]:
		out.write(" "+tool["name"])
	out.write("\n")
	# all
	out.write("all:")
	for lib in conf["libraries"]:
		if not lib["external"]:
			out.write(" "+lib["name"])
	out.write(" tools\n")
	# tools
	out.write("tools:")
	for tool in conf["tools"]:
		out.write(" "+tool["name"])
	out.write("\n")

	# generate libs
	for lib in conf["libraries"]:
		if not lib["external"]:
			makefile=os.path.join(obj_dir, "Makefile."+lib["name"])
			generateLibraryMakefile(conf, ctx, lib, makefile, obj_dir, os.path.join("..", ".."))
			out.write(lib["name"]+":")
			deps=depClosure(conf, lib["deps"], True)
			for dep in deps:
				out.write(" "+dep)
			out.write("\n")
			out.write("\t$(MAKE) -f "+"Makefile."+lib["name"]+"\n\n")
	
	# generate tools
	for tool in conf["tools"]:
		makefile=os.path.join(obj_dir, "Makefile."+tool["name"])
		generateToolMakefile(conf, ctx, tool, makefile, obj_dir, os.path.join("..", ".."))
		out.write(tool["name"]+":")
		deps=depClosure(conf, lib["deps"], True)
		for dep in deps:
			out.write(" "+dep)
		out.write("\n")
		out.write("\t$(MAKE) -f "+"Makefile."+ tool["name"]+"\n\n")

	out.write("clean:\n")
	for lib in conf["libraries"]:
		if not lib["external"]:
			out.write("\t$(MAKE) -f "+"Makefile."+lib["name"]+" clean\n")
	for tool in conf["tools"]:
		out.write("\t$(MAKE) -f "+"Makefile."+tool["name"]+" clean\n")


#	out.write("dep:\n")
#	for lib in conf["libraries"]:
#		generateDEPS(conf, ctx, lib, out, obj_dir, generateCPPFLAGS(conf, ctx, lib), filename)
#	for tool in conf["tools"]:
#		generateDEPS(conf, ctx, tool, out, obj_dir, generateCPPFLAGS(conf, ctx, tool), filename)
	out.close()

def generateMakefiles(ctx, source):
	for platform in ctx["platforms"]:
		conf=generateConfiguration(ctx, [source], platform)
		if ctx["platform"] in conf["platforms"]:			
			generateMakefile(conf, ctx)
		else:
			parseError("platform "+ctx["platform"]+" is unknown.")
			sys.stderr.write("available platforms:");
			for platform in conf["platforms"]:
				sys.stderr.write(platform)
	out=open("Makefile", "w")
	out.write(".phony: all clean");
	for platform in ctx["platforms"]:
		out.write(" "+platform);
	out.write("\n");
	out.write("all:");
	for platform in ctx["platforms"]:
		out.write(" "+platform);
	out.write("\n");
	for platform in ctx["platforms"]:
		out.write(platform+": build"+os.sep+platform+os.sep+"Makefile\n");
		out.write("\t@$(MAKE) -C build"+os.sep+platform+"\n");
	out.write("clean:\n")
	for platform in ctx["platforms"]:
		out.write("\t@$(MAKE) -C build"+os.sep+platform+" clean\n");	
	out.close()

def dumpEnvironment(ctx, includes, platform):
	conf=generateConfiguration(ctx, includes, platform)
	if platform in conf["platforms"]:			
		p=conf["platforms"][platform]
#		print(str(p))
		host=""
		if "host" in p:
			host="HOST=\""+p["host"]+"\" "
		sys.stdout.write("export CC=\""+p["cc"]+"\" "+
			 "export AR=\""+p["ar"]+"\" "+
			 "export LD=\""+p["ld"]+"\" "
			 "export CPPFLAGS=\""+p["cppflags"]+"\" "
			 "export CFLAGS=\""+p["cflags"]+"\" "+
			 host
			)
	else:
		parseError("platform "+platform+" is unknown.")
		sys.stderr.write("available platforms:");
		for platform in conf["platforms"]:
			sys.stderr.write(platform)	

def set_external_flag(ctx, lib, flag, value):
#	sys.stderr.write("SETTING "+lib+" "+flag+" "+str(value)+"\n")
	if lib in ctx["externals"]:
		ctx["externals"][lib][flag]=value
	else:
		ctx["externals"][lib]={flag: value}


def usage():
	print("usage:")
	print(" generate [--include \"Makefile.xml\"]")
	print("          [--config-dir \"config/\"]")
	print("          [--print-env \"linux-release-gcc\"]")

if __name__ == "__main__":	
	try:
		opts, args = getopt.getopt(sys.argv[1:], "h", ["help", "include=", "config-dir=", "print-env=", 
			"with-zlib-cppflags=", "with-zlib-ldflags=", "with-zlib=",
			"with-libxml-cppflags=", "with-libxml-ldflags=", "with-libxml="])
	except getopt.GetoptError, err:
		# print help information and exit:
		print str(err) # will print something like "option -a not recognized"
		usage()
		sys.exit(2)
	ctx={ "base": os.path.abspath(os.curdir), "root": os.path.abspath(os.curdir), "platform": "?-?-?", "platforms": [], "externals": {} }
	ctx["config"]=os.path.join(ctx["base"], "config")
	includes=[]
	dump_env=[]
	for o, a in opts:
		if o in ("-h", "--help"):
			usage()
			sys.exit(0)
		elif o in ("--include"):
			includes.append(a)
		elif o in ("--config-dir"):
			ctx["config"]=os.path.join(ctx["base"], a)
		elif o in ("--print-env"):
			args=[]
			dump_env=[a]
		elif o in ("--with-zlib"):
			set_external_flag(ctx, "zlib", "external", "yes"==a)
		elif o in ("--with-zlib-cppflags"):
			set_external_flag(ctx, "zlib", "cppflags", a)
		elif o in ("--with-zlib-ldflags"):
			set_external_flag(ctx, "zlib", "ldflags", a)
		elif o in ("--with-libxml"):
			set_external_flag(ctx, "xml", "external", "yes"==a)
		elif o in ("--with-libxml-cppflags"):
			set_external_flag(ctx, "xml", "cppflags", a)
		elif o in ("--with-libxml-ldflags"):
			set_external_flag(ctx, "xml", "ldflags", a)

	for platform in args:
		ctx["platforms"].append(platform)

	if 1==len(ctx["platforms"]) and platformSubseteqTest(ctx["platforms"][0], "win32-*-*") and 1==len(includes):
		generateWin32(ctx, includes[0])
	else:
		if not os.path.exists("build"):
			os.makedirs("build")
		f=open("build"+os.sep+"configure.ctx", "w")
		f.write(str(ctx))
		f.close()		
		for include in includes:
			generateMakefiles(ctx, include)

	for platform in dump_env:
		dumpEnvironment(ctx, includes, platform)
