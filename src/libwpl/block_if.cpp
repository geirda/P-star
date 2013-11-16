/*

-------------------------------------------------------------

Copyright (c) MMIII Atle Solbakken
atle@goliathdns.no

-------------------------------------------------------------

This file is part of P* (P-star).

P* is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

P* is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with P*.  If not, see <http://www.gnu.org/licenses/>.

-------------------------------------------------------------

*/

#include "debug.h"
#include "block_state.h"
#include "block_if.h"

int wpl_block_if::run(wpl_state *state, wpl_value *final_result) {
	int ret = WPL_OP_NO_RETURN;

	wpl_block_state *block_state = (wpl_block_state*) state;

	if (check_run(block_state)) {
		ret = wpl_block::run(state, final_result);
	}
	else if (next_else_if) {
		ret = block_state->run_next_else_if (next_else_if, final_result);
	}

	return ret;
}
