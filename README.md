# open62541-nodeset-exporter

## General

A C++ 20 project for exporting a nodeset to xml and other formats.
The main dependency of the project is the [Open62541](https://github.com/open62541/open62541) library. This library must
be pre-compiled and installed on the system. The supported library version is Open62541 1.3.x - 1.4.x.

The nodesetexporter library, using the client (and in the future, the server) of the Open62541 library, collects the
necessary data from the remote OPC UA server specified to the client and converts this data into an export file in XML
format. The nodesetexporter library only needs to provide the client handle in the connected state and a list of nodes
that need to be exported, then the library itself collects the necessary information, such as aliases, namespaces, node
attributes, types, etc.
The exported node structure can be loaded onto another server, for example, using
another [NodesetLoader](https://github.com/open62541/open62541-nodeset-loader) project.

The library does not currently support exporting custom types. This is planned for implementation.

The build for Windows has not been tested and is not supported but is planned for implementation.

## Contribution

The library is raw and requires additional testing and further development, so I will be glad to everyone who wants and
is able to participate in the development of the project. :)

## Features

Already done:

✅ Linux build support \
✅ Support Open62541 1.3.x and 1.4.x versions \
✅ Export of multiple nodes \
✅ Export to XML (verified using UANodeSet.xsd) \
✅ You can add other export modules using the IEncoder interface, not just XML (but it will be outside the standard) \
✅ Using the Open62541 Client to collect information for export \
✅ Export Aliases, Namespaces, UAObjects, UAObjectTypes, UAVariables, UAVariableTypes, UAReferenceTypes (only
HierarhicalReference), UADataTypes (without definition). Nodes with references. \
✅ Cli utility for exporting \
✅ Added experimental optional modes: ns0_custom_nodes_ready_to_work, flat_list_of_nodes, 
flat_list_of_nodes__create_missing_start_node, flat_list_of_nodes__allow_abstract_variable \  
✅ Added data values for XML. You can export node data values to XML. There are restrictions on the types of values 
that can be exported. All simple types are supported, as well as UA_NodeClass, StatusCode, UA_DateTime, UA_Guid, 
UA_String (ByteString), UA_NodeId, UA_ExpandedNodeId, UA_QualifiedName, UA_LocalizedText, and UA_DiagnosticInfo.  
MultidimensionalArrays of all specified types are also supported.  

Planned:

⭕ Using the Open62541 Server to collect information for export \
⭕ Support Definition (DataTypeDefinition) in UADataTypes \
⭕ Collect and export all custom data types \
⭕ Exporting UAView \
⭕ Windows build support \
⭕ More unit tests \
⭕ Upgrade to Conan 2

❌ UAMethods is not exported because we can't move the code \
❌ Nodes and types ns=0 are not exported (except custom nodes in ns=0 (**optional**))

## Dependencies

OS: Linux (Debian, Ubuntu, ...). Windows support is planned.

To build you will need installed in your system:

- python3
- pip
- conan 1
- git
- make
- cmake
- GCC (C++20 features are available since GCC 8, but development was carried out on GCC 12).
  https://gcc.gnu.org/projects/cxx-status.html
- pre-compiled and pre-installed open62541 version 1.3.x or 1.4.x (UA_ENABLE_JSON_ENCODING=ON needed)

Conan dependencies:

- boost/1.84.0
- tinyxml2/10.0.0
- fmt/10.2.1
- magic_enum/0.9.6
- libxmlpp/5.2.0 (only for tests)
- doctest/2.4.11 (only for tests)
- trompeloeil/47 (only for tests)

To build the environment, you can use the **install-build-deps.sh** script if you are working on Ubuntu 24 LTS (Noble)
version or add your version to the following part of the code, for example:

```bash
    case "${VERSION}" in
    *Focal* | *Jammy*) # <-- Here
```
But if you add support for previous OS versions, you will need to change some versions of Clang and GCC or other utilities in the script.

## How to build

In the shell you need to run the following commands:

```bash
git clone https://github.com/xydan83/open62541-nodeset-exporter.git
cd open62541-nodeset-exporter/
mkdir build
cd build
cmake ..
make -j

# for installation to the system
sudo make install
```

⚠️ It is not recommended to build their master branches. The best option is to switch to the desired release version after
cloning the repository and entering the folder.
`git checkout tags/v1.x.x`

### Build options

```
CMAKE_BUILD_TYPE - Set build type (Debug|Release|RelWithDebInfo|MinSizeRel). (default: Release)
NODESETEXPORTER_PERFORMANCE_TIMER_ENABLED - Set the option to enable code using a performance timer (default: OFF)
NODESETEXPORTER_CONAN_ENABLE - Use the Conan package manager to download, build, and install dependencies for specific versions. (default: ON)
If you also use Conan in your project, then set the value to OFF and provide all the necessary dependencies in your dependency file for Conan.
NODESETEXPORTER_BUILD_TESTS - This option allows you to disable the build of tests from the default build. (default: OFF)
NODESETEXPORTER_CLI_ENABLE - This option allows you to create a command utility that can export a set of nodes from the OPC UA Server node space. (default: OFF)
NODESETEXPORTER_OPEN62541_IS_SUBMODULE - If you want to add the open62541 library as a static submodule set ON. (default OFF)
BUILD_SHARED_LIBS - Allows you to build a shared library with dynamic linking instead of a library with static linking. (default: OFF)
OPEN62541_VERSIONS - What version of the Open62541 library are you using? Switch between "v1.3.x" "v1.4.x". (default: v1.3.x)
```

You can change the above settings in your project file (for example in CMakeLists.txt) using `set(*param* ON/OFF)` when
building via submodules or using variables `cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON ..` when building
via the command line.

### To pass tests

Since the tests try to cover the maximum number of different options for using the library, the tests are designed to
build the Open62541 library with the maximum number of zero node space types.
If you want to run tests, you must first build and install the Open62541 library with the following parameter:

```shell
-DUA_NAMESPACE_ZERO=FULL
```

## Examples

There are two example projects on how to use the library:

- An example of how to include and use the nodesetexporter and open62541 libraries together as submodules in your
  project. There: https://github.com/xydan83/open62541-nodeset-exporter-insource-demo
- An example of including and using nodesetexporter as a shared library.
  There: https://github.com/xydan83/open62541-nodeset-exporter-demo

## Cli utility

The library project has a separate command line utility project that can be used to export the node structure from a
OPC UA Server from a specific starting node. This utility has launch parameters.
This utility can also serve as an additional, more advanced example of using the library.

The utility project is located here: https://github.com/xydan83/open62541-nodeset-exporter/tree/master/apps

The assembly of the utility as part of the library can be enabled using the option NODESETEXPORTER_CLI_ENABLE.

### Parameters

```
Usage: ./cli_nodesetexporter [options]
Options:
  -h [ --help ]                         Show hints
  -v [ --version ]                      Show version
  -e [ --endpoint ] arg (=opc.tcp://localhost:4840)
                                        Endpoint to OPC UA Server
  -n [ --nodeids ] arg                  The IDs of the nodes from which the 
                                        export will be started. For example: 
                                        "ns=2;i=1" "ns=2;s=test"
  -f [ --file ] arg (=nodeset_export.xml)
                                        Path with filename to export
  -u [ --username ] arg                 Authentication username
  -p [ --password ] arg                 Authentication password
  --maxnpr arg (=0)                     Maximum size of the nodesToRead array
  --maxnpb arg (=0)                     Maximum size of the nodesToBrowse array
  --maxrpn arg (=0)                     Maximum number of references to return 
                                        for each starting Node specified in the
                                        request
  -t [ --timeout ] arg (=5000)          Response timeout in ms
  --perftimer arg (=0)                  Enable the performance timer 
                                        (true/false)
  --parent arg                          The parent node ID of all of the start 
                                        nodes, which is replaced by the custom 
                                        one for the binding. default: "i=85"
```

## Experimental optional modes:

**ns0_custom_nodes_ready_to_work** - Export user nodes located in the standard OPC UA space (ns=0).

**flat_list_of_nodes__is_enable** - Generate a flat list of nodes linked to a single start node. All related links
between nodes are deleted. The Objects [i=85] node from the standard OPC UA space can be specified as a start node. All
other nodes of the standard are prohibited. If the specified start node is missing on the server and the "
flat_list_of_nodes__create_missing_start_node" parameter is disabled,an export error will be returned.

**flat_list_of_nodes__create_missing_start_node** - Works in conjunction only with the activated parameter "
flat_list_of_nodes__is_enable". In case of activation, if the starting node does not exist (an error will occur when
trying to collect data from it), then such a node will be created as the "Object" class and bound to
the parent node specified in the "parent_start_node_replacer" parameter.
If all other node lists have one non-existent node, then such lists will all be bound to one created node.

**flat_list_of_nodes__allow_abstract_variable** - Works in conjunction with
"flat_list_of_nodes__create_missing_start_node" and "flat_list_of_nodes__is_enable". When enabled, adding two backlinks
of type "HasComponent" to nodes "i=63" and "i=58" thus allows using nodes of class "Variable" of abstract type.

If you want to use these modes, you can enable them in the `Options --> flat_list_of_nodes` structure in
`NodesetExporter.h`.

## License

MPL2.0: https://github.com/xydan83/open62541-nodeset-exporter/blob/master/LICENSE
