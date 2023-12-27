/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

//
//  main.cpp
//  antlr4-cpp-demo
//
//  Created by Mike Lischke on 13.03.16.
//

#include <iostream>

#include "antlr4-runtime.h"
#include "ExprLexer.h"
#include "ExprParser.h"


using namespace antlr4;

int main(int argc, const char * argv[]) {

    ANTLRInputStream input("10 + 20 * 40 /30 + 20 - 10");
    ExprLexer lexer(&input);
    CommonTokenStream tokens(&lexer);

    ExprParser parser(&tokens);
    auto *tree = parser.prog();

    auto s = tree->toStringTree(&parser);
    std::cout << "Parse Tree: " << s << std::endl;

    return 0;
}
