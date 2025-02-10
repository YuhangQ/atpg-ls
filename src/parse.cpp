#include "circuit.h"

#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <set>

using namespace atpg_ls;

void line2tokens(const std::string &line, std::vector<std::string> &tokens) {
    std::string token;
    for(char c : line) {
        if(c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
        if(c == '=' || c == ',' || c == '(' || c == ')') {
            if(!token.size()) continue;
            tokens.push_back(token);
            token.clear(); token += c;
            tokens.push_back(token);
            token.clear();
        } else {
            token += c;
        }
    }
    if(token.size()) {
        tokens.push_back(token);
    }
}

void Circuit::parse_from_file(const char *filename) {

    std::ifstream file(filename);
    if(!file.is_open()) {
        printf("Error while reading %s\n", filename);
        exit(1);
    }

    // buf 10MB
    file.rdbuf()->pubsetbuf(0, 10 * 1024 * 1024);

    std::vector<std::string> bufPO;

    std::string line;
    while(std::getline(file, line)) {
        if(line[0] == '#') continue;
        if(line == "\r" || line == "") continue;

        std::vector<std::string> tokens;

        line2tokens(line, tokens);
        // std::cout << line << std::endl;
        // std::cout << "tokens: ";
        // for(auto &token : tokens) {
        //     std::cout << "$" << token << "$ ";
        // }
        // std::cout << std::endl;        

        // gate
        if(tokens.size() >= 6 && tokens[1] == "=" && tokens[3] == "(" && tokens.back() == ")") {
            std::vector<std::string> ins;

            for(int i=4; i<tokens.size()-1; i+=2) {
                if(tokens[i] == ",") {
                    printf("Error while reading line: %s\n", line.c_str());
                    exit(1);
                }
                ins.push_back(tokens[i]);
            }
            for(int i=5; i<tokens.size()-1; i+=2) {
                if(tokens[i] != ",") {
                    printf("Error while reading line: %s\n", line.c_str());
                    exit(1);
                }
            }

            Gate* gate = new Gate();
            gate->name = tokens[0];
            gate->isPI = false;
            gate->isPO = false;

            for(auto &in : ins) {
                if(!name2gate.count(in)) {
                    printf("Error while reading file: gate %s used before defination.\n", in.c_str());
                    exit(1);
                }

                auto in_gate = name2gate[in];

                gate->fanins.push_back(in_gate);
                in_gate->fanouts.push_back(gate);
            }

            if(tokens[2] == "AND") { gate->type = Gate::AND; }
            else if(tokens[2] == "NAND") { gate->type = Gate::NAND; }
            else if(tokens[2] == "OR") { gate->type = Gate::OR; }
            else if(tokens[2] == "NOR") { gate->type = Gate::NOR; }
            else if(tokens[2] == "XOR") { gate->type = Gate::XOR; }
            else if(tokens[2] == "XNOR") { gate->type = Gate::XNOR; }
            else if(tokens[2] == "NOT") { gate->type = Gate::NOT; }
            else if(tokens[2] == "BUFF") { gate->type = Gate::BUF; }
            else if(tokens[2] == "BUF") { gate->type = Gate::BUF; }
            else {
                printf("Error while reading file: %s is not a valid gate.\n", tokens[2].c_str());
                exit(1);
            }

            name2gate.insert(std::make_pair(gate->name, gate));
            gates.push_back(gate);
        }
        // input
        else if(tokens.size() == 4 && tokens[0] == "INPUT" && tokens[1] == "(" && tokens[3] == ")") {
            Gate* gate = new Gate();
            gate->name = tokens[2];
            gate->type = Gate::INPUT;
            gate->isPI = true;
            gate->isPO = false;

            name2gate.insert(std::make_pair(gate->name, gate));
            gates.push_back(gate);
            PIs.push_back(gate);
        } 
        // gate
        else if(tokens.size() == 4 && tokens[0] == "OUTPUT" && tokens[1] == "(" && tokens[3] == ")") {
            bufPO.push_back(tokens[2]);
        }
        // invalid line
        else {
            printf("Error while reading line: %s\n", line.c_str());
            exit(1);
        }
    }

    // handle PO
    for(auto& po_name : bufPO) {
        if(!name2gate.count(po_name)) {
            printf("Error while reading file: po %s is not a exist gate.\n", po_name.c_str());
            exit(1);
        }

        Gate* po = name2gate[po_name];
        po->isPO = true;
        POs.push_back(po);
    }
}