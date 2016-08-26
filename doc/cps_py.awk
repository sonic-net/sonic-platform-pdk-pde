#!/usr/bin/gawk -f
#
# Copyright (c) 2015 Dell Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
# LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
# FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
#
# See the Apache Version 2.0 License for specific language governing
# permissions and limitations under the License.
#
BEGIN { indoc = 0 }
{
    if (index($1, "PyDoc_STRVAR") == 1) {
        if (0 == index($0, ");")) {

            printf ("def ");
            for (ix = 2; ix <= NF; ++ix) {
                pattern = "\"";
                str = $ix;
                res = gsub(pattern, "", str);
                printf ("%s ", str);
            }
            printf (":\n");
            print "    \"\"\""
            indoc = 1;
        }
    }
    else {
        if (1 == indoc) {
            str = $0;
            pattern = "\"";
            gsub(pattern, "", str);
            pattern = ");";
            gsub(pattern, "", str);
            print str;
            if (0 != index($0, ");")) {
                indoc = 0;
                print "    \"\"\""
            }
        }
    }

}
