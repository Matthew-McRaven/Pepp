#include <iostream>

#include "src/SvgDocument.hpp"

int main()
{
    std::cout << "Hello World!" << std::endl;

    Document doc{};

    doc.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\aa.svg");
    doc.saveAs("x:\\aa2.svg");

    Document doc2{"x:\\aa2.svg"};
    doc2.saveAs("x:\\aa3.svg");

    return 0;
}
