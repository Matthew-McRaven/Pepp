#include <iostream>

#include "src/SvgDocument.hpp"

//	private classes
#include "src/Timer.h"

int main()
{
    Timer<> t;
    t.start();
    Document doc{};

    doc.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\aa.svg");
    doc.saveAs("x:\\aa2.svg");
    t.finish();

    //Document doc2{"x:\\aa2.svg"};
    //doc2.saveAs("x:\\aa3.svg");

    std::cout << "Total Elapsed Time: " << t.elapsedTime() << std::endl;

    return 0;
}
