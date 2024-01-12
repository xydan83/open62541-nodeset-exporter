/* WARNING: This is a generated file.
 * Any manual changes will be overwritten. */

#include "ex_nodeset.h"


/* vPLC2 - ns=2;i=100 */

static UA_StatusCode function_ex_nodeset_0_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", "vPLC2");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description. Testing multiple start nodes.");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_OBJECT,
        UA_NODEID_NUMERIC(ns[2], 100LU),
        UA_NODEID_NUMERIC(ns[0], 85LU),
        UA_NODEID_NUMERIC(ns[0], 35LU),
        UA_QUALIFIEDNAME(ns[2], "vPLC2"),
        UA_NODEID_NUMERIC(ns[0], 61LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    return retVal;
}

static UA_StatusCode function_ex_nodeset_0_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[2], 100LU));
}

/* integer32 - ns=2;i=102 */

static UA_StatusCode function_ex_nodeset_1_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.minimumSamplingInterval = 0.000000;
    attr.userAccessLevel = 1;
    attr.accessLevel = 1;
    /* Value rank inherited */
    attr.valueRank = -2;
    attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
    UA_Int32* variablenode_ns_2_i_102_variant_DataContents = UA_Int32_new();
    if (!variablenode_ns_2_i_102_variant_DataContents)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_Int32_init(variablenode_ns_2_i_102_variant_DataContents);
    *variablenode_ns_2_i_102_variant_DataContents = (UA_Int32)9994;
    UA_Variant_setScalar(&attr.value, variablenode_ns_2_i_102_variant_DataContents, &UA_TYPES[UA_TYPES_INT32]);
    attr.displayName = UA_LOCALIZEDTEXT("", "integer32");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description integer32");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_VARIABLE,
        UA_NODEID_NUMERIC(ns[2], 102LU),
        UA_NODEID_NUMERIC(ns[2], 100LU),
        UA_NODEID_NUMERIC(ns[0], 47LU),
        UA_QUALIFIEDNAME(ns[2], "integer32"),
        UA_NODEID_NUMERIC(ns[0], 63LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    UA_Int32_delete(variablenode_ns_2_i_102_variant_DataContents);
    return retVal;
}

static UA_StatusCode function_ex_nodeset_1_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[2], 102LU));
}

/* boolean - ns=2;i=101 */

static UA_StatusCode function_ex_nodeset_2_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.minimumSamplingInterval = 0.000000;
    attr.userAccessLevel = 1;
    attr.accessLevel = 1;
    /* Value rank inherited */
    attr.valueRank = -2;
    attr.dataType = UA_NODEID_NUMERIC(ns[0], 1LU);
    UA_Boolean* variablenode_ns_2_i_101_variant_DataContents = UA_Boolean_new();
    if (!variablenode_ns_2_i_101_variant_DataContents)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_Boolean_init(variablenode_ns_2_i_101_variant_DataContents);
    *variablenode_ns_2_i_101_variant_DataContents = (UA_Boolean) true;
    UA_Variant_setScalar(&attr.value, variablenode_ns_2_i_101_variant_DataContents, &UA_TYPES[UA_TYPES_BOOLEAN]);
    attr.displayName = UA_LOCALIZEDTEXT("", "boolean");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description boolean");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_VARIABLE,
        UA_NODEID_NUMERIC(ns[2], 101LU),
        UA_NODEID_NUMERIC(ns[2], 100LU),
        UA_NODEID_NUMERIC(ns[0], 47LU),
        UA_QUALIFIEDNAME(ns[2], "boolean"),
        UA_NODEID_NUMERIC(ns[0], 63LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    UA_Boolean_delete(variablenode_ns_2_i_101_variant_DataContents);
    return retVal;
}

static UA_StatusCode function_ex_nodeset_2_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[2], 101LU));
}

/* vPLC1 - ns=1;i=1 */

static UA_StatusCode function_ex_nodeset_3_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", "vPLC1");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description vPLC1");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_OBJECT,
        UA_NODEID_NUMERIC(ns[1], 1LU),
        UA_NODEID_NUMERIC(ns[0], 85LU),
        UA_NODEID_NUMERIC(ns[0], 35LU),
        UA_QUALIFIEDNAME(ns[1], "vPLC1"),
        UA_NODEID_NUMERIC(ns[0], 61LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    return retVal;
}

static UA_StatusCode function_ex_nodeset_3_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 1LU));
}

/* myNewStaticObject1 - ns=1;i=5 */

static UA_StatusCode function_ex_nodeset_4_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", "myNewStaticObject1");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description myNewStaticObject1");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_OBJECT,
        UA_NODEID_NUMERIC(ns[1], 5LU),
        UA_NODEID_NUMERIC(ns[1], 1LU),
        UA_NODEID_NUMERIC(ns[0], 47LU),
        UA_QUALIFIEDNAME(ns[1], "myNewStaticObject1"),
        UA_NODEID_NUMERIC(ns[0], 58LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    return retVal;
}

static UA_StatusCode function_ex_nodeset_4_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 5LU));
}

/* myNewStaticObject1_1 - ns=1;i=9 */

static UA_StatusCode function_ex_nodeset_5_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", "myNewStaticObject1_1");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description myNewStaticObject1_1");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_OBJECT,
        UA_NODEID_NUMERIC(ns[1], 9LU),
        UA_NODEID_NUMERIC(ns[1], 5LU),
        UA_NODEID_NUMERIC(ns[0], 47LU),
        UA_QUALIFIEDNAME(ns[1], "myNewStaticObject1_1"),
        UA_NODEID_NUMERIC(ns[0], 58LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    return retVal;
}

static UA_StatusCode function_ex_nodeset_5_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 9LU));
}

/* myNewStaticObject1_1_1 - ns=1;i=15 */

static UA_StatusCode function_ex_nodeset_6_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", "myNewStaticObject1_1_1");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description myNewStaticObject1_1_1");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_OBJECT,
        UA_NODEID_NUMERIC(ns[1], 15LU),
        UA_NODEID_NUMERIC(ns[1], 9LU),
        UA_NODEID_NUMERIC(ns[0], 47LU),
        UA_QUALIFIEDNAME(ns[1], "myNewStaticObject1_1_1"),
        UA_NODEID_NUMERIC(ns[0], 58LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    return retVal;
}

static UA_StatusCode function_ex_nodeset_6_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15LU));
}

/* MyProperty2 - ns=1;i=16 */

static UA_StatusCode function_ex_nodeset_7_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.minimumSamplingInterval = 0.000000;
    attr.userAccessLevel = 1;
    attr.accessLevel = 1;
    /* Value rank inherited */
    attr.valueRank = -2;
    attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
    UA_String* variablenode_ns_1_i_16_variant_DataContents = UA_String_new();
    if (!variablenode_ns_1_i_16_variant_DataContents)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_String_init(variablenode_ns_1_i_16_variant_DataContents);
    *variablenode_ns_1_i_16_variant_DataContents = UA_STRING_ALLOC("some text in property");
    UA_Variant_setScalar(&attr.value, variablenode_ns_1_i_16_variant_DataContents, &UA_TYPES[UA_TYPES_STRING]);
    attr.displayName = UA_LOCALIZEDTEXT("", "MyProperty2");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description MyProperty2");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_VARIABLE,
        UA_NODEID_NUMERIC(ns[1], 16LU),
        UA_NODEID_NUMERIC(ns[1], 15LU),
        UA_NODEID_NUMERIC(ns[0], 46LU),
        UA_QUALIFIEDNAME(ns[1], "MyProperty2"),
        UA_NODEID_NUMERIC(ns[0], 68LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    UA_String_delete(variablenode_ns_1_i_16_variant_DataContents);
    return retVal;
}

static UA_StatusCode function_ex_nodeset_7_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 16LU));
}

/* TestMethod - ns=1;i=12 */

static UA_StatusCode function_ex_nodeset_8_begin(UA_Server* server, UA_UInt16* ns)
{
#ifdef UA_ENABLE_METHODCALLS
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_MethodAttributes attr = UA_MethodAttributes_default;
    attr.executable = true;
    attr.userExecutable = true;
    attr.displayName = UA_LOCALIZEDTEXT("", "TestMethod");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description TestMethod");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_METHOD,
        UA_NODEID_NUMERIC(ns[1], 12LU),
        UA_NODEID_NUMERIC(ns[1], 9LU),
        UA_NODEID_NUMERIC(ns[0], 47LU),
        UA_QUALIFIEDNAME(ns[1], "TestMethod"),
        UA_NODEID_NULL,
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_METHODATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    return retVal;
#else
    return UA_STATUSCODE_GOOD;
#endif /* UA_ENABLE_METHODCALLS */
}

static UA_StatusCode function_ex_nodeset_8_finish(UA_Server* server, UA_UInt16* ns)
{
#ifdef UA_ENABLE_METHODCALLS
    return UA_Server_addMethodNode_finish(server, UA_NODEID_NUMERIC(ns[1], 12LU), NULL, 0, NULL, 0, NULL);
#else
    return UA_STATUSCODE_GOOD;
#endif /* UA_ENABLE_METHODCALLS */
}

/* OutputArguments - ns=1;i=14 */

static UA_StatusCode function_ex_nodeset_9_begin(UA_Server* server, UA_UInt16* ns)
{
#ifdef UA_ENABLE_METHODCALLS
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.minimumSamplingInterval = 0.000000;
    attr.userAccessLevel = 1;
    attr.accessLevel = 1;
    attr.valueRank = 0;
    attr.dataType = UA_NODEID_NUMERIC(ns[0], 296LU);
    UA_Argument variablenode_ns_1_i_14_variant_DataContents[1];

    UA_init(&variablenode_ns_1_i_14_variant_DataContents[0], &UA_TYPES[UA_TYPES_ARGUMENT]);
    variablenode_ns_1_i_14_variant_DataContents[0].name = UA_STRING("");
    variablenode_ns_1_i_14_variant_DataContents[0].dataType = UA_NODEID_NUMERIC(ns[0], 8LU);
    variablenode_ns_1_i_14_variant_DataContents[0].valueRank = (UA_Int32)-2;
    UA_Variant_setArray(&attr.value, &variablenode_ns_1_i_14_variant_DataContents, (UA_Int32)1, &UA_TYPES[UA_TYPES_ARGUMENT]);
    attr.displayName = UA_LOCALIZEDTEXT("", "OutputArguments");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description OutputArguments");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_VARIABLE,
        UA_NODEID_NUMERIC(ns[1], 14LU),
        UA_NODEID_NUMERIC(ns[1], 12LU),
        UA_NODEID_NUMERIC(ns[0], 46LU),
        UA_QUALIFIEDNAME(ns[0], "OutputArguments"),
        UA_NODEID_NUMERIC(ns[0], 68LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;

    return retVal;
#else
    return UA_STATUSCODE_GOOD;
#endif /* UA_ENABLE_METHODCALLS */
}

static UA_StatusCode function_ex_nodeset_9_finish(UA_Server* server, UA_UInt16* ns)
{
#ifdef UA_ENABLE_METHODCALLS
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 14LU));
#else
    return UA_STATUSCODE_GOOD;
#endif /* UA_ENABLE_METHODCALLS */
}

/* InputArguments - ns=1;i=13 */

static UA_StatusCode function_ex_nodeset_10_begin(UA_Server* server, UA_UInt16* ns)
{
#ifdef UA_ENABLE_METHODCALLS
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.minimumSamplingInterval = 0.000000;
    attr.userAccessLevel = 1;
    attr.accessLevel = 1;
    attr.valueRank = 0;
    attr.dataType = UA_NODEID_NUMERIC(ns[0], 296LU);
    UA_Argument variablenode_ns_1_i_13_variant_DataContents[1];

    UA_init(&variablenode_ns_1_i_13_variant_DataContents[0], &UA_TYPES[UA_TYPES_ARGUMENT]);
    variablenode_ns_1_i_13_variant_DataContents[0].name = UA_STRING("");
    variablenode_ns_1_i_13_variant_DataContents[0].dataType = UA_NODEID_NUMERIC(ns[0], 8LU);
    variablenode_ns_1_i_13_variant_DataContents[0].valueRank = (UA_Int32)-2;
    UA_Variant_setArray(&attr.value, &variablenode_ns_1_i_13_variant_DataContents, (UA_Int32)1, &UA_TYPES[UA_TYPES_ARGUMENT]);
    attr.displayName = UA_LOCALIZEDTEXT("", "InputArguments");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description InputArguments");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_VARIABLE,
        UA_NODEID_NUMERIC(ns[1], 13LU),
        UA_NODEID_NUMERIC(ns[1], 12LU),
        UA_NODEID_NUMERIC(ns[0], 46LU),
        UA_QUALIFIEDNAME(ns[0], "InputArguments"),
        UA_NODEID_NUMERIC(ns[0], 68LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;

    return retVal;
#else
    return UA_STATUSCODE_GOOD;
#endif /* UA_ENABLE_METHODCALLS */
}

static UA_StatusCode function_ex_nodeset_10_finish(UA_Server* server, UA_UInt16* ns)
{
#ifdef UA_ENABLE_METHODCALLS
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 13LU));
#else
    return UA_STATUSCODE_GOOD;
#endif /* UA_ENABLE_METHODCALLS */
}

/* MyProperty - ns=1;i=11 */

static UA_StatusCode function_ex_nodeset_11_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.minimumSamplingInterval = 0.000000;
    attr.userAccessLevel = 1;
    attr.accessLevel = 1;
    /* Value rank inherited */
    attr.valueRank = -2;
    attr.dataType = UA_NODEID_NUMERIC(ns[0], 11LU);
    UA_Double* variablenode_ns_1_i_11_variant_DataContents = UA_Double_new();
    if (!variablenode_ns_1_i_11_variant_DataContents)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_Double_init(variablenode_ns_1_i_11_variant_DataContents);
    *variablenode_ns_1_i_11_variant_DataContents = (UA_Double)0.21;
    UA_Variant_setScalar(&attr.value, variablenode_ns_1_i_11_variant_DataContents, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.displayName = UA_LOCALIZEDTEXT("", "MyProperty");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description MyProperty");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_VARIABLE,
        UA_NODEID_NUMERIC(ns[1], 11LU),
        UA_NODEID_NUMERIC(ns[1], 9LU),
        UA_NODEID_NUMERIC(ns[0], 46LU),
        UA_QUALIFIEDNAME(ns[1], "MyProperty"),
        UA_NODEID_NUMERIC(ns[0], 68LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    UA_Double_delete(variablenode_ns_1_i_11_variant_DataContents);
    return retVal;
}

static UA_StatusCode function_ex_nodeset_11_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 11LU));
}

/* static_param1 - ns=1;i=10 */

static UA_StatusCode function_ex_nodeset_12_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.minimumSamplingInterval = 0.000000;
    attr.userAccessLevel = 1;
    attr.accessLevel = 1;
    /* Value rank inherited */
    attr.valueRank = -2;
    attr.dataType = UA_NODEID_NUMERIC(ns[0], 8LU);
    UA_Int64* variablenode_ns_1_i_10_variant_DataContents = UA_Int64_new();
    if (!variablenode_ns_1_i_10_variant_DataContents)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_Int64_init(variablenode_ns_1_i_10_variant_DataContents);
    *variablenode_ns_1_i_10_variant_DataContents = (UA_Int64)532;
    UA_Variant_setScalar(&attr.value, variablenode_ns_1_i_10_variant_DataContents, &UA_TYPES[UA_TYPES_INT64]);
    attr.displayName = UA_LOCALIZEDTEXT("", "static_param1");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description static_param1");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_VARIABLE,
        UA_NODEID_NUMERIC(ns[1], 10LU),
        UA_NODEID_NUMERIC(ns[1], 9LU),
        UA_NODEID_NUMERIC(ns[0], 47LU),
        UA_QUALIFIEDNAME(ns[1], "static_param1"),
        UA_NODEID_NUMERIC(ns[0], 63LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    UA_Int64_delete(variablenode_ns_1_i_10_variant_DataContents);
    return retVal;
}

static UA_StatusCode function_ex_nodeset_12_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 10LU));
}

/* static_param3 - ns=1;i=8 */

static UA_StatusCode function_ex_nodeset_13_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.minimumSamplingInterval = 0.000000;
    attr.userAccessLevel = 1;
    attr.accessLevel = 1;
    /* Value rank inherited */
    attr.valueRank = -2;
    attr.dataType = UA_NODEID_NUMERIC(ns[0], 11LU);
    UA_Double* variablenode_ns_1_i_8_variant_DataContents = UA_Double_new();
    if (!variablenode_ns_1_i_8_variant_DataContents)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_Double_init(variablenode_ns_1_i_8_variant_DataContents);
    *variablenode_ns_1_i_8_variant_DataContents = (UA_Double)123.32;
    UA_Variant_setScalar(&attr.value, variablenode_ns_1_i_8_variant_DataContents, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.displayName = UA_LOCALIZEDTEXT("", "static_param3");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description static_param3");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_VARIABLE,
        UA_NODEID_NUMERIC(ns[1], 8LU),
        UA_NODEID_NUMERIC(ns[1], 5LU),
        UA_NODEID_NUMERIC(ns[0], 47LU),
        UA_QUALIFIEDNAME(ns[1], "static_param3"),
        UA_NODEID_NUMERIC(ns[0], 63LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    UA_Double_delete(variablenode_ns_1_i_8_variant_DataContents);
    return retVal;
}

static UA_StatusCode function_ex_nodeset_13_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 8LU));
}

/* static_text_param2 - ns=1;i=7 */

static UA_StatusCode function_ex_nodeset_14_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.minimumSamplingInterval = 0.000000;
    attr.userAccessLevel = 1;
    attr.accessLevel = 1;
    /* Value rank inherited */
    attr.valueRank = -2;
    attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
    UA_String* variablenode_ns_1_i_7_variant_DataContents = UA_String_new();
    if (!variablenode_ns_1_i_7_variant_DataContents)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_String_init(variablenode_ns_1_i_7_variant_DataContents);
    *variablenode_ns_1_i_7_variant_DataContents = UA_STRING_ALLOC("some text");
    UA_Variant_setScalar(&attr.value, variablenode_ns_1_i_7_variant_DataContents, &UA_TYPES[UA_TYPES_STRING]);
    attr.displayName = UA_LOCALIZEDTEXT("", "static_text_param2");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description static_text_param2");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_VARIABLE,
        UA_NODEID_NUMERIC(ns[1], 7LU),
        UA_NODEID_NUMERIC(ns[1], 5LU),
        UA_NODEID_NUMERIC(ns[0], 47LU),
        UA_QUALIFIEDNAME(ns[1], "static_text_param2"),
        UA_NODEID_NUMERIC(ns[0], 63LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    UA_String_delete(variablenode_ns_1_i_7_variant_DataContents);
    return retVal;
}

static UA_StatusCode function_ex_nodeset_14_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 7LU));
}

/* static_param1 - ns=1;i=6 */

static UA_StatusCode function_ex_nodeset_15_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.minimumSamplingInterval = 0.000000;
    attr.userAccessLevel = 1;
    attr.accessLevel = 1;
    /* Value rank inherited */
    attr.valueRank = -2;
    attr.dataType = UA_NODEID_NUMERIC(ns[0], 8LU);
    UA_Int64* variablenode_ns_1_i_6_variant_DataContents = UA_Int64_new();
    if (!variablenode_ns_1_i_6_variant_DataContents)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_Int64_init(variablenode_ns_1_i_6_variant_DataContents);
    *variablenode_ns_1_i_6_variant_DataContents = (UA_Int64)311;
    UA_Variant_setScalar(&attr.value, variablenode_ns_1_i_6_variant_DataContents, &UA_TYPES[UA_TYPES_INT64]);
    attr.displayName = UA_LOCALIZEDTEXT("", "static_param1");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description static_param1");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_VARIABLE,
        UA_NODEID_NUMERIC(ns[1], 6LU),
        UA_NODEID_NUMERIC(ns[1], 5LU),
        UA_NODEID_NUMERIC(ns[0], 47LU),
        UA_QUALIFIEDNAME(ns[1], "static_param1"),
        UA_NODEID_NUMERIC(ns[0], 63LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    UA_Int64_delete(variablenode_ns_1_i_6_variant_DataContents);
    return retVal;
}

static UA_StatusCode function_ex_nodeset_15_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 6LU));
}

/* pressure - ns=1;i=3 */

static UA_StatusCode function_ex_nodeset_16_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.minimumSamplingInterval = 0.000000;
    attr.userAccessLevel = 1;
    attr.accessLevel = 1;
    /* Value rank inherited */
    attr.valueRank = -2;
    attr.dataType = UA_NODEID_NUMERIC(ns[0], 11LU);
    UA_Double* variablenode_ns_1_i_3_variant_DataContents = UA_Double_new();
    if (!variablenode_ns_1_i_3_variant_DataContents)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_Double_init(variablenode_ns_1_i_3_variant_DataContents);
    *variablenode_ns_1_i_3_variant_DataContents = (UA_Double)49.52257;
    UA_Variant_setScalar(&attr.value, variablenode_ns_1_i_3_variant_DataContents, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.displayName = UA_LOCALIZEDTEXT("", "pressure");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description pressure");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_VARIABLE,
        UA_NODEID_NUMERIC(ns[1], 3LU),
        UA_NODEID_NUMERIC(ns[1], 1LU),
        UA_NODEID_NUMERIC(ns[0], 47LU),
        UA_QUALIFIEDNAME(ns[1], "pressure"),
        UA_NODEID_NUMERIC(ns[0], 63LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    UA_Double_delete(variablenode_ns_1_i_3_variant_DataContents);
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 3LU), UA_NODEID_NUMERIC(ns[0], 47LU), UA_EXPANDEDNODEID_NUMERIC(ns[1], 5LU), false);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    return retVal;
}

static UA_StatusCode function_ex_nodeset_16_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 3LU));
}

/* myNewStaticObject1_2 - ns=1;i=17 */

static UA_StatusCode function_ex_nodeset_17_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", "myNewStaticObject1_2");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description myNewStaticObject1_2");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_OBJECT,
        UA_NODEID_NUMERIC(ns[1], 17LU),
        UA_NODEID_NUMERIC(ns[1], 5LU),
        UA_NODEID_NUMERIC(ns[0], 47LU),
        UA_QUALIFIEDNAME(ns[1], "myNewStaticObject1_2"),
        UA_NODEID_NUMERIC(ns[0], 58LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    return retVal;
}

static UA_StatusCode function_ex_nodeset_17_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 17LU));
}

/* static_param3 - ns=1;i=20 */

static UA_StatusCode function_ex_nodeset_18_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.minimumSamplingInterval = 1000.000000;
    attr.userAccessLevel = 1;
    attr.accessLevel = 1;
    /* Value rank inherited */
    attr.valueRank = -2;
    attr.dataType = UA_NODEID_NUMERIC(ns[0], 11LU);
    UA_Double* variablenode_ns_1_i_20_variant_DataContents = UA_Double_new();
    if (!variablenode_ns_1_i_20_variant_DataContents)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_Double_init(variablenode_ns_1_i_20_variant_DataContents);
    *variablenode_ns_1_i_20_variant_DataContents = (UA_Double)5883.04;
    UA_Variant_setScalar(&attr.value, variablenode_ns_1_i_20_variant_DataContents, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.displayName = UA_LOCALIZEDTEXT("", "static_param3");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description static_param3");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_VARIABLE,
        UA_NODEID_NUMERIC(ns[1], 20LU),
        UA_NODEID_NUMERIC(ns[1], 17LU),
        UA_NODEID_NUMERIC(ns[0], 47LU),
        UA_QUALIFIEDNAME(ns[1], "static_param3"),
        UA_NODEID_NUMERIC(ns[0], 63LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    UA_Double_delete(variablenode_ns_1_i_20_variant_DataContents);
    return retVal;
}

static UA_StatusCode function_ex_nodeset_18_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 20LU));
}

/* static_text_param2 - ns=1;i=19 */

static UA_StatusCode function_ex_nodeset_19_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.minimumSamplingInterval = 0.000000;
    attr.userAccessLevel = 1;
    attr.accessLevel = 1;
    /* Value rank inherited */
    attr.valueRank = -2;
    attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
    UA_String* variablenode_ns_1_i_19_variant_DataContents = UA_String_new();
    if (!variablenode_ns_1_i_19_variant_DataContents)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_String_init(variablenode_ns_1_i_19_variant_DataContents);
    *variablenode_ns_1_i_19_variant_DataContents = UA_STRING_ALLOC("Try get this");
    UA_Variant_setScalar(&attr.value, variablenode_ns_1_i_19_variant_DataContents, &UA_TYPES[UA_TYPES_STRING]);
    attr.displayName = UA_LOCALIZEDTEXT("", "static_text_param2");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description static_text_param2");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_VARIABLE,
        UA_NODEID_NUMERIC(ns[1], 19LU),
        UA_NODEID_NUMERIC(ns[1], 17LU),
        UA_NODEID_NUMERIC(ns[0], 47LU),
        UA_QUALIFIEDNAME(ns[1], "static_text_param2"),
        UA_NODEID_NUMERIC(ns[0], 63LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    UA_String_delete(variablenode_ns_1_i_19_variant_DataContents);
    return retVal;
}

static UA_StatusCode function_ex_nodeset_19_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 19LU));
}

/* static_param1 - ns=1;i=18 */

static UA_StatusCode function_ex_nodeset_20_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.minimumSamplingInterval = 0.000000;
    attr.userAccessLevel = 1;
    attr.accessLevel = 1;
    /* Value rank inherited */
    attr.valueRank = -2;
    attr.dataType = UA_NODEID_NUMERIC(ns[0], 8LU);
    UA_Int64* variablenode_ns_1_i_18_variant_DataContents = UA_Int64_new();
    if (!variablenode_ns_1_i_18_variant_DataContents)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_Int64_init(variablenode_ns_1_i_18_variant_DataContents);
    *variablenode_ns_1_i_18_variant_DataContents = (UA_Int64)9953;
    UA_Variant_setScalar(&attr.value, variablenode_ns_1_i_18_variant_DataContents, &UA_TYPES[UA_TYPES_INT64]);
    attr.displayName = UA_LOCALIZEDTEXT("", "static_param1");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description static_param1");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_VARIABLE,
        UA_NODEID_NUMERIC(ns[1], 18LU),
        UA_NODEID_NUMERIC(ns[1], 17LU),
        UA_NODEID_NUMERIC(ns[0], 47LU),
        UA_QUALIFIEDNAME(ns[1], "static_param1"),
        UA_NODEID_NUMERIC(ns[0], 63LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    UA_Int64_delete(variablenode_ns_1_i_18_variant_DataContents);
    return retVal;
}

static UA_StatusCode function_ex_nodeset_20_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 18LU));
}

/* pumpsetting - ns=1;i=4 */

static UA_StatusCode function_ex_nodeset_21_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.minimumSamplingInterval = 0.000000;
    attr.userAccessLevel = 1;
    attr.accessLevel = 1;
    /* Value rank inherited */
    attr.valueRank = -2;
    attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
    UA_String* variablenode_ns_1_i_4_variant_DataContents = UA_String_new();
    if (!variablenode_ns_1_i_4_variant_DataContents)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_String_init(variablenode_ns_1_i_4_variant_DataContents);
    *variablenode_ns_1_i_4_variant_DataContents = UA_STRING_ALLOC("speed");
    UA_Variant_setScalar(&attr.value, variablenode_ns_1_i_4_variant_DataContents, &UA_TYPES[UA_TYPES_STRING]);
    attr.displayName = UA_LOCALIZEDTEXT("", "pumpsetting");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description pumpsetting");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_VARIABLE,
        UA_NODEID_NUMERIC(ns[1], 4LU),
        UA_NODEID_NUMERIC(ns[1], 1LU),
        UA_NODEID_NUMERIC(ns[0], 47LU),
        UA_QUALIFIEDNAME(ns[1], "pumpsetting"),
        UA_NODEID_NUMERIC(ns[0], 63LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    UA_String_delete(variablenode_ns_1_i_4_variant_DataContents);
    return retVal;
}

static UA_StatusCode function_ex_nodeset_21_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 4LU));
}

/* HasChild_test - ns=1;i=30 */

static UA_StatusCode function_ex_nodeset_22_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_ReferenceTypeAttributes attr = UA_ReferenceTypeAttributes_default;
    attr.isAbstract = true;
    attr.displayName = UA_LOCALIZEDTEXT("", "HasChild_test");
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_REFERENCETYPE,
        UA_NODEID_NUMERIC(ns[1], 30LU),
        UA_NODEID_NUMERIC(ns[0], 33LU),
        UA_NODEID_NUMERIC(ns[0], 45LU),
        UA_QUALIFIEDNAME(ns[1], "HasChild_test"),
        UA_NODEID_NULL,
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 30LU), UA_NODEID_NUMERIC(ns[0], 35LU), UA_EXPANDEDNODEID_NUMERIC(ns[1], 1LU), false);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    return retVal;
}

static UA_StatusCode function_ex_nodeset_22_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 30LU));
}

/* HasEventSource_test - ns=1;i=29 */

static UA_StatusCode function_ex_nodeset_23_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_ReferenceTypeAttributes attr = UA_ReferenceTypeAttributes_default;
    attr.inverseName = UA_LOCALIZEDTEXT("", "EventSourceOf");
    attr.displayName = UA_LOCALIZEDTEXT("", "HasEventSource_test");
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_REFERENCETYPE,
        UA_NODEID_NUMERIC(ns[1], 29LU),
        UA_NODEID_NUMERIC(ns[0], 33LU),
        UA_NODEID_NUMERIC(ns[0], 45LU),
        UA_QUALIFIEDNAME(ns[1], "HasEventSource_test"),
        UA_NODEID_NULL,
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 29LU), UA_NODEID_NUMERIC(ns[0], 35LU), UA_EXPANDEDNODEID_NUMERIC(ns[1], 1LU), false);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    return retVal;
}

static UA_StatusCode function_ex_nodeset_23_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 29LU));
}

/* SomeReferences_test - ns=1;i=28 */

static UA_StatusCode function_ex_nodeset_24_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_ReferenceTypeAttributes attr = UA_ReferenceTypeAttributes_default;
    attr.isAbstract = true;
    attr.symmetric = true;
    attr.displayName = UA_LOCALIZEDTEXT("", "SomeReferences_test");
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_REFERENCETYPE,
        UA_NODEID_NUMERIC(ns[1], 28LU),
        UA_NODEID_NUMERIC(ns[0], 33LU),
        UA_NODEID_NUMERIC(ns[0], 45LU),
        UA_QUALIFIEDNAME(ns[1], "SomeReferences_test"),
        UA_NODEID_NULL,
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 28LU), UA_NODEID_NUMERIC(ns[0], 35LU), UA_EXPANDEDNODEID_NUMERIC(ns[1], 1LU), false);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    return retVal;
}

static UA_StatusCode function_ex_nodeset_24_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 28LU));
}

/* FolderType_test - ns=1;i=24 */

static UA_StatusCode function_ex_nodeset_25_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", "FolderType_test");
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_OBJECTTYPE,
        UA_NODEID_NUMERIC(ns[1], 24LU),
        UA_NODEID_NUMERIC(ns[0], 58LU),
        UA_NODEID_NUMERIC(ns[0], 45LU),
        UA_QUALIFIEDNAME(ns[1], "FolderType_test"),
        UA_NODEID_NULL,
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 24LU), UA_NODEID_NUMERIC(ns[0], 35LU), UA_EXPANDEDNODEID_NUMERIC(ns[1], 1LU), false);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    return retVal;
}

static UA_StatusCode function_ex_nodeset_25_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 24LU));
}

/* AggregateConfigurationType_test - ns=1;i=23 */

static UA_StatusCode function_ex_nodeset_26_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.isAbstract = true;
    attr.displayName = UA_LOCALIZEDTEXT("", "AggregateConfigurationType_test");
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_OBJECTTYPE,
        UA_NODEID_NUMERIC(ns[1], 23LU),
        UA_NODEID_NUMERIC(ns[0], 58LU),
        UA_NODEID_NUMERIC(ns[0], 45LU),
        UA_QUALIFIEDNAME(ns[1], "AggregateConfigurationType_test"),
        UA_NODEID_NULL,
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 23LU), UA_NODEID_NUMERIC(ns[0], 46LU), UA_EXPANDEDNODEID_NUMERIC(ns[0], 11188LU), true);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 23LU), UA_NODEID_NUMERIC(ns[0], 46LU), UA_EXPANDEDNODEID_NUMERIC(ns[0], 11189LU), true);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 23LU), UA_NODEID_NUMERIC(ns[0], 46LU), UA_EXPANDEDNODEID_NUMERIC(ns[0], 11190LU), true);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 23LU), UA_NODEID_NUMERIC(ns[0], 46LU), UA_EXPANDEDNODEID_NUMERIC(ns[0], 11191LU), true);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 23LU), UA_NODEID_NUMERIC(ns[0], 35LU), UA_EXPANDEDNODEID_NUMERIC(ns[1], 1LU), false);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    return retVal;
}

static UA_StatusCode function_ex_nodeset_26_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 23LU));
}

/* EnumValues - ns=1;i=22 */

static UA_StatusCode function_ex_nodeset_27_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.minimumSamplingInterval = 0.000000;
    attr.userAccessLevel = 1;
    attr.accessLevel = 1;
    attr.valueRank = 1;
    attr.arrayDimensionsSize = 1;
    UA_UInt32 arrayDimensions[1];
    arrayDimensions[0] = 3;
    attr.arrayDimensions = &arrayDimensions[0];
    attr.dataType = UA_NODEID_NUMERIC(ns[0], 7594LU);
    UA_EnumValueType variablenode_ns_1_i_22_variant_DataContents[3];

    UA_init(&variablenode_ns_1_i_22_variant_DataContents[0], &UA_TYPES[UA_TYPES_ENUMVALUETYPE]);
    variablenode_ns_1_i_22_variant_DataContents[0].value = (UA_Int64)1;
    variablenode_ns_1_i_22_variant_DataContents[0].displayName = UA_LOCALIZEDTEXT("", "Mandatory");
    variablenode_ns_1_i_22_variant_DataContents[0].description = UA_LOCALIZEDTEXT("", "The BrowseName must appear in all instances of the type.");

    UA_init(&variablenode_ns_1_i_22_variant_DataContents[1], &UA_TYPES[UA_TYPES_ENUMVALUETYPE]);
    variablenode_ns_1_i_22_variant_DataContents[1].value = (UA_Int64)2;
    variablenode_ns_1_i_22_variant_DataContents[1].displayName = UA_LOCALIZEDTEXT("", "Optional");
    variablenode_ns_1_i_22_variant_DataContents[1].description = UA_LOCALIZEDTEXT("", "The BrowseName may appear in an instance of the type.");

    UA_init(&variablenode_ns_1_i_22_variant_DataContents[2], &UA_TYPES[UA_TYPES_ENUMVALUETYPE]);
    variablenode_ns_1_i_22_variant_DataContents[2].value = (UA_Int64)3;
    variablenode_ns_1_i_22_variant_DataContents[2].displayName = UA_LOCALIZEDTEXT("", "Constraint");
    variablenode_ns_1_i_22_variant_DataContents[2].description = UA_LOCALIZEDTEXT("", "The modelling rule defines a constraint and the BrowseName is not used in an instance of the type.");
    UA_Variant_setArray(&attr.value, &variablenode_ns_1_i_22_variant_DataContents, (UA_Int32)3, &UA_TYPES[UA_TYPES_ENUMVALUETYPE]);
    attr.displayName = UA_LOCALIZEDTEXT("", "EnumValues");
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_VARIABLE,
        UA_NODEID_NUMERIC(ns[1], 22LU),
        UA_NODEID_NUMERIC(ns[1], 1LU),
        UA_NODEID_NUMERIC(ns[0], 47LU),
        UA_QUALIFIEDNAME(ns[0], "EnumValues"),
        UA_NODEID_NUMERIC(ns[0], 68LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;


    return retVal;
}

static UA_StatusCode function_ex_nodeset_27_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 22LU));
}

/* Union - ns=1;i=21 */

static UA_StatusCode function_ex_nodeset_28_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.isAbstract = true;
    attr.displayName = UA_LOCALIZEDTEXT("", "Union");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description Union");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_DATATYPE,
        UA_NODEID_NUMERIC(ns[1], 21LU),
        UA_NODEID_NUMERIC(ns[0], 22LU),
        UA_NODEID_NUMERIC(ns[0], 45LU),
        UA_QUALIFIEDNAME(ns[1], "Union"),
        UA_NODEID_NULL,
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_DATATYPEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 21LU), UA_NODEID_NUMERIC(ns[0], 35LU), UA_EXPANDEDNODEID_NUMERIC(ns[1], 1LU), false);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    return retVal;
}

static UA_StatusCode function_ex_nodeset_28_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 21LU));
}

/* Union concrete - ns=1;i=31 */

static UA_StatusCode function_ex_nodeset_29_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", "Union concrete");
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_DATATYPE,
        UA_NODEID_NUMERIC(ns[1], 31LU),
        UA_NODEID_NUMERIC(ns[1], 21LU),
        UA_NODEID_NUMERIC(ns[0], 45LU),
        UA_QUALIFIEDNAME(ns[1], "Union_concrete"),
        UA_NODEID_NULL,
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_DATATYPEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    return retVal;
}

static UA_StatusCode function_ex_nodeset_29_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 31LU));
}

/* KeyValuePair_test - ns=1;i=32 */

static UA_StatusCode function_ex_nodeset_30_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", "KeyValuePair_test");
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_DATATYPE,
        UA_NODEID_NUMERIC(ns[1], 32LU),
        UA_NODEID_NUMERIC(ns[0], 22LU),
        UA_NODEID_NUMERIC(ns[0], 45LU),
        UA_QUALIFIEDNAME(ns[0], "KeyValuePair_test"),
        UA_NODEID_NULL,
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_DATATYPEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 32LU), UA_NODEID_NUMERIC(ns[0], 45LU), UA_EXPANDEDNODEID_NUMERIC(ns[1], 31LU), false);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    return retVal;
}

static UA_StatusCode function_ex_nodeset_30_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 32LU));
}

/* temperature - ns=1;i=2 */

static UA_StatusCode function_ex_nodeset_31_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.minimumSamplingInterval = 0.000000;
    attr.userAccessLevel = 1;
    attr.accessLevel = 1;
    /* Value rank inherited */
    attr.valueRank = -2;
    attr.dataType = UA_NODEID_NUMERIC(ns[0], 11LU);
    UA_Double* variablenode_ns_1_i_2_variant_DataContents = UA_Double_new();
    if (!variablenode_ns_1_i_2_variant_DataContents)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_Double_init(variablenode_ns_1_i_2_variant_DataContents);
    *variablenode_ns_1_i_2_variant_DataContents = (UA_Double)45.52951;
    UA_Variant_setScalar(&attr.value, variablenode_ns_1_i_2_variant_DataContents, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.displayName = UA_LOCALIZEDTEXT("", "temperature");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
    attr.description = UA_LOCALIZEDTEXT("", "Description temperature");
#endif
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_VARIABLE,
        UA_NODEID_NUMERIC(ns[1], 2LU),
        UA_NODEID_NUMERIC(ns[1], 1LU),
        UA_NODEID_NUMERIC(ns[0], 47LU),
        UA_QUALIFIEDNAME(ns[1], "temperature"),
        UA_NODEID_NUMERIC(ns[0], 63LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    UA_Double_delete(variablenode_ns_1_i_2_variant_DataContents);
    return retVal;
}

static UA_StatusCode function_ex_nodeset_31_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 2LU));
}

/* SamplingIntervalDiagnosticsArrayType_test - ns=1;i=27 */

static UA_StatusCode function_ex_nodeset_32_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;
    attr.valueRank = 1;
    attr.arrayDimensionsSize = 1;
    UA_UInt32 arrayDimensions[1];
    arrayDimensions[0] = 0;
    attr.arrayDimensions = &arrayDimensions[0];
    attr.dataType = UA_NODEID_NUMERIC(ns[0], 856LU);
    attr.displayName = UA_LOCALIZEDTEXT("", "SamplingIntervalDiagnosticsArrayType_test");
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_VARIABLETYPE,
        UA_NODEID_NUMERIC(ns[1], 27LU),
        UA_NODEID_NUMERIC(ns[0], 63LU),
        UA_NODEID_NUMERIC(ns[0], 45LU),
        UA_QUALIFIEDNAME(ns[1], "SamplingIntervalDiagnosticsArrayType_test"),
        UA_NODEID_NUMERIC(ns[0], 0LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_VARIABLETYPEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 27LU), UA_NODEID_NUMERIC(ns[0], 47LU), UA_EXPANDEDNODEID_NUMERIC(ns[0], 12779LU), true);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 27LU), UA_NODEID_NUMERIC(ns[0], 47LU), UA_EXPANDEDNODEID_NUMERIC(ns[1], 2LU), false);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    return retVal;
}

static UA_StatusCode function_ex_nodeset_32_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 27LU));
}

/* DataTypeDescriptionType_test - ns=1;i=26 */

static UA_StatusCode function_ex_nodeset_33_begin(UA_Server* server, UA_UInt16* ns)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;
    /* Value rank inherited */
    attr.valueRank = -2;
    attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
    attr.displayName = UA_LOCALIZEDTEXT("", "DataTypeDescriptionType_test");
    retVal |= UA_Server_addNode_begin(
        server,
        UA_NODECLASS_VARIABLETYPE,
        UA_NODEID_NUMERIC(ns[1], 26LU),
        UA_NODEID_NUMERIC(ns[0], 63LU),
        UA_NODEID_NUMERIC(ns[0], 45LU),
        UA_QUALIFIEDNAME(ns[1], "DataTypeDescriptionType_test"),
        UA_NODEID_NUMERIC(ns[0], 0LU),
        (const UA_NodeAttributes*)&attr,
        &UA_TYPES[UA_TYPES_VARIABLETYPEATTRIBUTES],
        NULL,
        NULL);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 26LU), UA_NODEID_NUMERIC(ns[0], 46LU), UA_EXPANDEDNODEID_NUMERIC(ns[0], 104LU), true);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 26LU), UA_NODEID_NUMERIC(ns[0], 46LU), UA_EXPANDEDNODEID_NUMERIC(ns[0], 105LU), true);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 26LU), UA_NODEID_NUMERIC(ns[0], 47LU), UA_EXPANDEDNODEID_NUMERIC(ns[1], 2LU), false);
    if (retVal != UA_STATUSCODE_GOOD)
        return retVal;
    return retVal;
}

static UA_StatusCode function_ex_nodeset_33_finish(UA_Server* server, UA_UInt16* ns)
{
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 26LU));
}

UA_StatusCode ex_nodeset(UA_Server* server)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    /* Use namespace ids generated by the server */
    UA_UInt16 ns[3];
    ns[0] = UA_Server_addNamespace(server, "http://opcfoundation.org/UA/");
    ns[1] = UA_Server_addNamespace(server, "http://test/nodes/1");
    ns[2] = UA_Server_addNamespace(server, "http://test/nodes/2");

    /* Load custom datatype definitions into the server */
    if ((retVal = function_ex_nodeset_0_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_1_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_2_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_3_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_4_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_5_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_6_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_7_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_8_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_9_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_10_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_11_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_12_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_13_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_14_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_15_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_16_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_17_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_18_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_19_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_20_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_21_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_22_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_22_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_23_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_23_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_24_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_24_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_25_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_26_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_27_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_28_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_29_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_30_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_31_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_32_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_33_begin(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_33_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_32_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_31_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_30_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_29_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_28_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_27_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_26_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_25_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_21_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_20_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_19_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_18_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_17_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_16_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_15_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_14_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_13_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_12_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_11_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_10_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_9_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_8_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_7_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_6_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_5_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_4_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_3_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_2_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_1_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    if ((retVal = function_ex_nodeset_0_finish(server, ns)) != UA_STATUSCODE_GOOD)
        return retVal;
    return retVal;
}
