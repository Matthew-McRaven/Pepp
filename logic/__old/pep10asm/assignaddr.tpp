#include "assignaddr.hpp"
#include "masm/project/image.hpp"
// Each image in the project must have its own
template <typename addr_size_t>
auto masm::backend::assign_image(std::shared_ptr<masm::project::project<addr_size_t> >& project, 
	std::shared_ptr<masm::elf::image<addr_size_t> >& image,
	std::list<masm::backend::region<addr_size_t> > control_script
) -> bool
{
	using tls_ptr_t = std::shared_ptr<masm::elf::top_level_section<addr_size_t> >;
	// Keep track of which sections *have not* been matched by our control script.
	std::list<tls_ptr_t> unmatched_sections;
	std::copy(image->sections.begin(), image->sections.end(), std::back_inserter(unmatched_sections));

	std::list<std::tuple<addr_size_t, std::list<tls_ptr_t>>> unmatched_sections;
}