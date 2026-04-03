#pragma once

#include "TableBuilder.h"

#include <iostream>
#include <iomanip>
#include <sstream>

class ParseTablePrinter
{
public:
    static void PrintDetailedRows(const std::vector<ParseTableRow>& rows)
    {
        if (rows.empty())
        {
            return;
        }

        const size_t idxW = 6;
        const size_t kindW = 12;
        const size_t nameW = 20;
        const size_t guidesW = 28;
        const size_t ptrW = 8;

        std::ostringstream sep;
        sep << "+" << std::string(idxW + 2, '-')
            << "+" << std::string(kindW + 2, '-')
            << "+" << std::string(nameW + 2, '-')
            << "+" << std::string(guidesW + 2, '-')
            << "+" << std::string(ptrW + 2, '-')
            << "+" << std::string(ptrW + 2, '-') << "+\n";

        std::cout << "\nDeterministic LL(1) table model:\n";
        std::cout << sep.str();

        std::cout << "| " << std::left << std::setw(idxW) << "idx" << " "
                  << "| " << std::left << std::setw(kindW) << "kind" << " "
                  << "| " << std::left << std::setw(nameW) << "name" << " "
                  << "| " << std::left << std::setw(guidesW) << "first_guides" << " "
                  << "| " << std::left << std::setw(ptrW) << "group" << " "
                  << "| " << std::left << std::setw(ptrW) << "next" << " |\n";

        std::cout << sep.str();

        for (const auto& row : rows)
        {
            std::string kindStr;
            switch (row.kind)
            {
                case ParseTableRow::Kind::NonterminalHeader:
                    kindStr = "NT-header";
                    break;
                case ParseTableRow::Kind::RhsTerminal:
                    kindStr = "RHS-term";
                    break;
                case ParseTableRow::Kind::RhsNonterminal:
                    kindStr = "RHS-NT";
                    break;
                case ParseTableRow::Kind::RhsEmpty:
                    kindStr = "RHS-empty";
                    break;
            }

            std::string guidesStr;
            if (!row.guides.empty())
            {
                bool first = true;
                for (const auto& g : row.guides)
                {
                    if (!first) guidesStr += ",";
                    guidesStr += g;
                    first = false;
                }
            }

            std::cout << "| " << std::setw(idxW) << row.idx << " "
                      << "| " << std::setw(kindW) << kindStr << " "
                      << "| " << std::setw(nameW) << row.name << " "
                      << "| " << std::setw(guidesW) << guidesStr << " "
                      << "| " << std::setw(ptrW) << (row.groupPointer == -1 ? "N/A" : std::to_string(row.groupPointer)) << " "
                      << "| " << std::setw(ptrW) << (row.nextPointer == -1 ? "N/A" : std::to_string(row.nextPointer)) << " |\n";
        }

        std::cout << sep.str();
    }
};