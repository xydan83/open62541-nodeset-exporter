//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_XMLHELPERFUNCTIONS_H
#define NODESETEXPORTER_XMLHELPERFUNCTIONS_H

#include <gsl/gsl>
#include <libxml++/libxml++.h> // Used for XML validation with XSD schema and node access

#include <sstream>

/**
 * @brief Node XML Parameter Validation Function
 * @param log_message Log output
 * @param node Node to be checked
 * @param cmp_node_name Verifiable hostname
 * @param cmp_node_text Verifiable text inside the node
 * @param cmp_attrs Verifiable set of attributes within the node
 */
static void CheckXMLNode( // NOLINT(misc-no-recursion)
    std::string& log_message,
    const xmlpp::Node* node,
    std::string cmp_node_name,
    std::string cmp_node_text = std::string(""),
    std::map<std::string, std::string> cmp_attrs = std::map<std::string, std::string>())
{
    Expects(node != nullptr);

    const auto* const nodeText = dynamic_cast<const xmlpp::TextNode*>(node);
    const auto* const nodeComment = dynamic_cast<const xmlpp::CommentNode*>(node);

    if (((nodeText != nullptr) && nodeText->is_white_space())) // Ignoring indentation
    {
        CheckXMLNode(log_message, node, std::move(cmp_node_name), std::move(cmp_node_text), std::move(cmp_attrs)); // recursive
        return;
    };
    const auto nodename = node->get_name();

    if ((nodeText == nullptr) && (nodeComment == nullptr) && !nodename.empty())
    {
        const auto namespace_prefix = node->get_namespace_prefix();
        std::string buf;
        buf += "Node name = ";
        if (!namespace_prefix.empty())
        {
            buf += namespace_prefix + ":";
        }
        buf += nodename;
        log_message += buf + "\n";
        if (cmp_node_name != nodename)
        {
            throw std::runtime_error("cmp_node_name (" + cmp_node_name + ") != nodename (" + nodename + ")");
        }
    }

    const auto* nodeElement = dynamic_cast<const xmlpp::Element*>(node);
    if (!cmp_attrs.empty() || nodeElement != nullptr)
    {
        if (nodeElement == nullptr)
        {
            throw std::runtime_error("There should be a ATTRIBUTES, but it's empty");
        }
        if (nodeElement->get_attributes().size() != cmp_attrs.size())
        {
            throw std::runtime_error(
                "nodeElement->get_attributes().size() (" + std::to_string(nodeElement->get_attributes().size()) + ") != cmp_attrs.size() (" + std::to_string(cmp_attrs.size()) + ")");
        }
    }

    if (nodeText != nullptr)
    {
        log_message += "Text Node =  \"" + nodeText->get_content() + "\"\n";
        if (cmp_node_text != nodeText->get_content())
        {
            throw std::runtime_error("cmp_node_text (" + cmp_node_text + ") != nodeText->get_content() (" + nodeText->get_content() + ")");
        }
    }
    else if (nodeComment != nullptr)
    {
        log_message += "Comment Node = " + nodeComment->get_content() + "\n";
    }
    else if (nodeElement != nullptr)
    {
        log_message += "     line = " + std::to_string(node->get_line()) + ", path = " + node->get_path() + ", namespace prefix: " + node->get_namespace_prefix()
                       + ", namespace: " + node->get_namespace_uri() + "\n";

        for (const auto& attribute : nodeElement->get_attributes())
        {
            const auto namespace_prefix = attribute->get_namespace_prefix();
            std::string buf;
            buf += "Attribute ";
            if (!namespace_prefix.empty())
            {
                buf += namespace_prefix + ":";
            }
            buf += attribute->get_name() + " = " + attribute->get_value();
            log_message += buf + "\n";
            std::string attr_ch;
            attr_ch = cmp_attrs.at(attribute->get_name());
            if (attr_ch != attribute->get_value())
            {
                throw std::runtime_error("attr_ch (" + attr_ch + ") != attribute->get_value() (" + attribute->get_value() + ")");
            }
        }
        for (const auto& child : node->get_children())
        {
            log_message += "Child node name: " + child->get_name() + "\n";
            const auto* const ch_text = dynamic_cast<const xmlpp::TextNode*>(child);
            const auto* const ch_comment = dynamic_cast<const xmlpp::CommentNode*>(child);
            if (!cmp_node_text.empty())
            {
                if (ch_text == nullptr)
                {
                    throw std::runtime_error("There should be a TEXT NODE, but it's empty");
                }
            }
            if (ch_text != nullptr && !ch_text->is_white_space())
            {
                // Checking the Text Node
                CheckXMLNode(log_message, child, cmp_node_name, cmp_node_text, std::map<std::string, std::string>()); // recursive
            }
            else if (ch_comment != nullptr)
            {
                // Show comment content
                CheckXMLNode(log_message, child, cmp_node_name, "", std::map<std::string, std::string>()); // recursive
            }
        }
    }
}

/**
 * @brief Function to search for the XML node in the DOM of the model with parsing and validation of the buf buffer.
 * @param xpath Search query in XPath format.
 * @param parser DOM parser object.
 * @param validator Validator object.
 * @param buf Buffer with XML where you want to find the node.
 * @return List of XML node objects.
 */
static xmlpp::Attribute::NodeSet GetFindXMLNode(const std::string& xpath, xmlpp::DomParser& parser, xmlpp::XsdValidator& validator, const std::stringstream& buf)
{
    const std::string out_xml(buf.str());
    parser.parse_memory(out_xml);
    validator.validate(parser.get_document()); // Schematic Validation
    auto* pNode = parser.get_document()->get_root_node();
    xmlpp::Node::PrefixNsMap nsMap = {{"xmlns", pNode->get_namespace_uri()}};
    return pNode->find(xpath, nsMap);
}

/**
 * @brief Function to search for an XML node in the DOM model.
 * @param xpath Search query in XPath format.
 * @param parser DOM parser object.
 * @return List of XML node objects.
 */
static std::vector<xmlpp::Node*> GetFindXMLNode(const std::string& xpath, xmlpp::DomParser& parser)
{
    auto* pNode = parser.get_document()->get_root_node();
    xmlpp::Node::PrefixNsMap nsMap = {{"xmlns", pNode->get_namespace_uri()}};
    return pNode->find(xpath, nsMap);
}

#endif // NODESETEXPORTER_XMLHELPERFUNCTIONS_H
