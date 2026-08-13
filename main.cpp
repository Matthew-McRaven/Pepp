#include <iostream>

#include "src/SvgDocument.hpp"

//	private classes
#include "src/Timer.h"

int main()
{
    Timer<> t;
    t.start();
    Document doc{};

    //  Works anim3.svg, USStates.svg (88k)-though CDATA does not work.
    doc.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\aa_rect.svg");
    //doc.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\car.svg"); //  500k file
    //doc.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\USStates.svg"); // Works!

    auto &svg = doc.documentElement();
    svg.setDesc("Desc from program"s);
    svg.setMetadata("Meta from program"s);
    svg.setTitle("Title from program"s);

    doc.saveAs("x:\\aa_rect2.svg");
    t.finish();

    //Document doc2{"x:\\aa2.svg"};
    //doc2.saveAs("x:\\aa3.svg");

    std::cout << "Total Elapsed Time: " << t.elapsedTime() << std::endl;

    return 0;
}
