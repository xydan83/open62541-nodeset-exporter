#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
#
# !/usr/bin/env python3

# todo Replace all namespaces after selecting the library from the project

import csv
import argparse
from pathlib import Path
from io import open

parser = argparse.ArgumentParser()
parser.add_argument('--nodeids_path', required=False, help='path to NodeIds.csv file', default='NodeIds.csv')
parser.add_argument('--path_to_header', required=False, help='path to output header file', default='DatatypeAliases.h')
args = parser.parse_args()

full_nodids_file_path = Path(args.nodeids_path)
full_header_output_file_path = Path(args.path_to_header)

print("Path to NodeIds.csv file - ", full_nodids_file_path)
print("Path to output header file - ", full_header_output_file_path)

output_filename = str(full_header_output_file_path).split('/')[-1:][0].split('.')[0]

with open(full_nodids_file_path, newline='') as nodeids_file:
    reader = csv.DictReader(nodeids_file, fieldnames=['Alias', 'NodeId', 'TypeOfData'])
    print("NodeID with aliases and data types:")
    for row in reader:
        if row['TypeOfData'] in ['DataType', 'ReferenceType']:
            print('Alias: ', row['Alias'], ', NodeId: i=', row['NodeId'], ', TypeOfData: ', row['TypeOfData'])

    with open(full_header_output_file_path, "wt", encoding='utf8') as fh:
        def printh(string):
            print(string, end=u'\n', file=fh)


        #########################
        # Print the header file #
        #########################
        printh(u'''//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
//  Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//  

//********************************************
//* Auto Generation - Do Not Manually Change *
//********************************************

#ifndef NODESETEXPORTER_COMMON_''' + output_filename.upper() + '''_H
#define NODESETEXPORTER_COMMON_''' + output_filename.upper() + '''_H

#include <map>
#include <string>

namespace nodesetexporter::''' + output_filename.replace('_', '').lower() + '''
{
/**
 * @brief Associative container with aliased data types  
 */
const std::map<std::uint32_t, std::string> data_type_aliases{''')
        nodeids_file.seek(0)
        for row in reader:
            if row['TypeOfData'] in ['DataType']:
                printh(u"\t{%s, \"%s\"}, // %s" % (row['NodeId'], row['Alias'], row['TypeOfData']))

        printh(u'''};

/**
 * @brief Associative container with alias types of references  
 */
const std::map<std::uint32_t, std::string> reference_type_aliases{''')
        nodeids_file.seek(0)
        for row in reader:
            if row['TypeOfData'] in ['ReferenceType']:
                printh(u"\t{%s, \"%s\"}, // %s" % (row['NodeId'], row['Alias'], row['TypeOfData']))

        printh(u'''};
} // namespace nodesetexporter::''' + output_filename.replace('_', '').lower() + '''
#endif // NODESETEXPORTER_COMMON_''' + output_filename.upper() + '''_H''')
