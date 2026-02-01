/*
 * Copyright (c) 2023-2025 J. Stanley Warford, Matthew McRaven
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
#include <catch.hpp>
#include <kitty/kitty.hpp>
#include <unordered_set>
static constinit const uint32_t num_vars = 3;
TEST_CASE("Sanity Tests for Kitty", "[scope:lg1][kind:unit][arch:*]") {
  SECTION("NPN") {
    /* truth table type in this example */
    using truth_table = kitty::static_truth_table<num_vars>;

    /* set to store all NPN representatives */
    std::unordered_set<truth_table, kitty::hash<truth_table>> classes;

    /* initialize truth table (constant 0) */
    truth_table tt;

    do {
      /* apply NPN canonization and add resulting representative to set */
      const auto res = kitty::exact_npn_canonization(tt);
      classes.insert(std::get<0>(res));

      /* increment truth table */
      kitty::next_inplace(tt);
    } while (!kitty::is_const0(tt));
    CHECK(classes.size() == 14);
    std::cout << "[i] enumerated " << (1 << (1 << tt.num_vars())) << " functions into " << classes.size() << " classes."
              << std::endl;
  }
  SECTION("NPN map enumeration") {
    static_assert(num_vars <= 5, "number of variables is limited to 5");

    /* truth table type in this example */
    using truth_table = kitty::static_truth_table<num_vars>;

    /* set to store all NPN representatives (dynamic to store bits on heap) */
    kitty::dynamic_truth_table map(truth_table::NumBits);

    /* invert bits: 1 means not classified yet */
    std::transform(map.cbegin(), map.cend(), map.begin(), [](auto word) { return ~word; });

    /* hash set to store all NPN classes */
    std::unordered_set<truth_table, kitty::hash<truth_table>> classes;

    /* start from 0 */
    int64_t index = 0;
    truth_table tt;

    while (index != -1) {
      /* create truth table from index value */
      kitty::create_from_words(tt, &index, &index + 1);

      /* apply NPN canonization and add resulting representative to set;
         while canonization, mark all encountered truth tables in map
       */
      const auto res =
          kitty::exact_npn_canonization(tt, [&map](const auto &tt) { kitty::clear_bit(map, *tt.cbegin()); });
      classes.insert(std::get<0>(res));
      /* find next non-classified truth table */
      index = find_first_one_bit(map);
    }
    CHECK(classes.size() == 14);
    std::cout << "[i] enumerated " << map.num_bits() << " functions into " << classes.size() << " classes."
              << std::endl;
  }
  SECTION("gf16_inverse") {
    /* This is the Boolean chain described in Fig. 1 of [J. Boyar and R. Peralta,
       SEC (2012), 287-298] */
    std::vector<std::string> chain{
        "x5 = x3 ^ x4",   "x6 = x1 & x3",   "x7 = x2 ^ x6",   "x8 = x1 ^ x2",   "x9 = x4 ^ x6",  "x10 = x8 & x9",
        "x11 = x5 & x7",  "x12 = x1 & x4",  "x13 = x8 & x12", "x14 = x8 ^ x13", "x15 = x2 & x3", "x16 = x5 & x15",
        "x17 = x5 ^ x16", "x18 = x6 ^ x17", "x19 = x4 ^ x11", "x20 = x6 ^ x14", "x21 = x2 ^ x10"};

    /* create truth tables */
    std::vector<kitty::static_truth_table<4>> steps;
    kitty::create_multiple_from_chain(4, steps, chain);

    /* output functions are in the last four steps */
    std::vector<kitty::static_truth_table<4>> y{steps[17], steps[18], steps[19], steps[20]};

    /* for each output ... */
    std::string golden[] = {"af90", "f360", "e4c6", "8aec"};
    for (auto i = 0; i < 4; ++i) {
      /* print hex representation of function */
      std::cout << "y" << (i + 1) << " = " << kitty::to_hex(y[i]) << "\n";
      CHECK(kitty::to_hex(y[i]) == golden[i]);

      // print PPRM (algebraic normal form) as in Sect. 3 of the above cited paper
      const auto cubes = esop_from_pprm(y[i]);
      kitty::print_cubes(cubes, 4);
    }

    std::cout << std::flush;
  }
  SECTION("ISOP") {
    /* we initialize TT using a lambda, since we do not know its size yet */
    kitty::dynamic_truth_table tt(10);
    kitty::create_random(tt);

    /* compute ISOP */
    const auto cubes = kitty::isop(tt);

    /* print ISOP */
    for (auto cube : cubes) {
      cube.print(tt.num_vars());
      std::cout << std::endl;
    }
  }
  SECTION("Spectral Enum") {
    /* truth table type in this example */
    using truth_table = kitty::static_truth_table<num_vars>;

    /* set to store all NPN and spectral representatives */
    using truth_table_set = std::unordered_set<truth_table, kitty::hash<truth_table>>;
    truth_table_set classes_npn, classes;

    /* initialize truth table (constant 0) */
    truth_table tt;

    do {
      /* apply NPN canonization and add resulting representative to set */
      const auto f_npn = std::get<0>(kitty::exact_npn_canonization(tt));
      const truth_table_set::const_iterator it = classes_npn.find(f_npn);
      if (it == classes_npn.end()) {
        classes_npn.insert(f_npn);
        classes.insert(kitty::exact_spectral_canonization(tt));
      }

      /* increment truth table */
      kitty::next_inplace(tt);
    } while (!kitty::is_const0(tt));

    std::cout << "[i] enumerated " << (1 << (1 << tt.num_vars())) << " functions into " << classes.size()
              << " classes.\n[i] spectrums:\n";
    CHECK(classes.size() == 3);

    for (const auto &r : classes) {
      const auto spectrum = kitty::rademacher_walsh_spectrum(r);
      std::copy(spectrum.begin(), spectrum.end(), std::ostream_iterator<int32_t>(std::cout, " "));
      std::cout << "\n";
    }
  }
  SECTION("Spectral Fuller") {
    /* This example is based on Algorithm 4.2.1 in the PhD thesis
     * "Analysis of Affine Equivalent Boolean Functions for Cryptography"
     * by J.E. Fuller (Queensland University of Technology)
     */
    using truth_table = kitty::static_truth_table<num_vars>;

    const auto start = std::chrono::steady_clock::now();
    std::vector<truth_table> fs(1);
    fuller_neighborhood_enumeration(fs, [](const auto &tt) { return exact_spectral_canonization(tt); });
    const auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "Found " << fs.size() << " classes in " << duration.count() << " s.\n";
    CHECK(fs.size() == 3);
  }
}
