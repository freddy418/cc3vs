/**

Copyright 2007
Georgia Tech Research Corporation
Atlanta, GA  30332-0415
All Rights Reserved

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

   * Redistributions of source code must retain the above copyright
   * notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above
   * copyright notice, this list of conditions and the following
   * disclaimer in the documentation and/or other materials provided
   * with the distribution.

   * Neither the name of the Goergia Tech Research Coporation nor the
   * names of its contributors may be used to endorse or promote
   * products derived from this software without specific prior
   * written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#include <cstdio>
#include <cstdlib>
 //#include <libxml/xmlmemory.h>
 //#include <libxml/parser.h>
#include "config_parser.h"
#include "dytan.h"

/**
 * Parses "sources" child of the xml configuration
 */
void parseSources(xmlDocPtr doc, xmlNodePtr cur, config *conf)
{
    xmlChar *txt, *subtext;
    xmlNodePtr sub;
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if((!xmlStrcmp(cur->name, (const xmlChar *)"source"))) {
            source src;
            txt = xmlGetProp ( cur, (const xmlChar *)"type" );
            if (txt) {
                fprintf(log, "sources type = %s:\n", txt );
                string type((char *)txt);
                src.type = type;
                xmlFree( txt );
                sub = cur->xmlChildrenNode;
                while (sub != NULL) {
                    if (sub->type == XML_ELEMENT_NODE) {
                        fprintf(log, "\t\t %s: ", sub->name);
                        subtext = xmlNodeListGetString(doc, sub->xmlChildrenNode, 1); 
                        fprintf(log, " %s\n", subtext);
                        fflush(log);
                        string details((char *)subtext);
                        src.details.push_back(details);
                        xmlFree(subtext);
                    }
                    sub = sub->next;
                }
            }
            else {
                fprintf(stderr, "Sources: Document not structured properly");
            }
            conf->sources.push_back(src);
        }
        cur = cur->next;
    }
}

/**
 * Parses "Propagation" child of the configuration 
 */
void parsePropagation(xmlDocPtr doc, xmlNodePtr cur, config *conf)
{
    xmlChar *key;
    propagation prop;
    prop.dataflow = false;
    prop.controlflow = false;
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
	if((!xmlStrcmp(cur->name, (const xmlChar *)"dataflow"))) {
	    key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	    fprintf(log, "\ndataflow :%s", key);
            prop.dataflow = (!strcmp((char *)key, "true") ? true: false);
	    xmlFree(key);
	}
        else if ((!xmlStrcmp(cur->name, (const xmlChar *)"controlflow"))) {
	    key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	    fprintf(log, "\ncontrolflow :%s", key);
            prop.controlflow = (!strcmp((char *)key, "true") ? true: false);
	    xmlFree(key);
	}
	cur = cur->next;
    }
    conf->prop = prop;
}

/**
 * Parses "Sinks" child of the configuration
 */
void parseSinks(xmlDocPtr doc, xmlNodePtr cur, config *conf)
{
    xmlChar *txt;
    xmlNodePtr sub, subsub;
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if((!xmlStrcmp(cur->name, (const xmlChar *)"sink"))) {
            sub = cur->xmlChildrenNode;
            while (sub != NULL) {
                if  ((!xmlStrcmp(sub->name, (const xmlChar *)"id"))) {
                    txt = xmlNodeListGetString(doc, sub->xmlChildrenNode, 1);
                    fprintf(log, "id = %s\n", txt);
                    xmlFree(txt);
                } else if (!xmlStrcmp(sub->name, (const xmlChar *)"location")) {
                    txt = xmlGetProp( sub, (const xmlChar *)"type");
                    if (txt) {
                        fprintf(log, "location type = %s:\n", txt );
                        xmlFree( txt );
                        subsub = sub->xmlChildrenNode;
                        while (subsub != NULL) {
                            if (subsub->type == XML_ELEMENT_NODE) {
                                fprintf(log, "\t\t %s ", subsub->name);
                                txt = xmlNodeListGetString(doc, subsub->xmlChildrenNode, 1); 
                                fprintf(log, " %s\n", txt);
                                xmlFree(txt);
                            }
                            subsub = subsub->next;
                        }
                    }
                }
                sub = sub->next;
            }
        } 
        cur = cur->next;
    }
}

#define CONFIGFILE "config.xml"

/**
 * Parse a simple XML document, using libxml2
 *
 */
int parseConfig( int argc, char **argv, config *conf ){

    xmlDocPtr doc;
    xmlNodePtr cur, sub;

    if( !(doc = xmlParseFile( CONFIGFILE ) ) ){
        fprintf( stderr, "Document not parsed successfully.\n" );
        xmlFreeDoc( doc );
        return( -1 );
    }
 
    if( !(cur = xmlDocGetRootElement( doc ) ) ){
        fprintf( stderr, "Document has no root element.\n" );
        xmlFreeDoc( doc );
        return( -1 );
    }
    if( xmlStrcmp( cur->name, (const xmlChar *) "dytan-config" ) ){
        fprintf( stderr, "Document of the wrong type, root node != dytan-config\n" );
        xmlFreeDoc( doc );
        return( -1 );
    }
 
    sub = cur->children;
    while( sub != NULL ){
        if( (!xmlStrcmp(sub->name, (const xmlChar *)"sources" ) ) ){
            parseSources(doc, sub, conf);
        }
        else if( (!xmlStrcmp(sub->name, (const xmlChar *)"propagation" ) ) ) {
	    parsePropagation(doc, sub, conf);
        }
        else if( (!xmlStrcmp(sub->name, (const xmlChar *)"sinks" ) ) ) {
            parseSinks(doc, sub, conf);
        }
        sub = sub->next;
    }
 
    fflush(log);
    xmlFreeDoc( doc );
    return 0;
}

