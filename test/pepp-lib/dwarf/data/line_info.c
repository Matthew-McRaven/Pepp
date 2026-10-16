/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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

// Source for line_info.elf. Rebuild with `make` in this directory after any edit since tests depend on line numbers.
static int square(int x) { return x * x; }

static int sum_of_squares(int n) {
  int total = 0;
  for (int i = 1; i <= n; i++) total += square(i);
  return total;
}

int main(void) {
  volatile int result = sum_of_squares(4);
  return result == 30 ? 0 : 1;
}
