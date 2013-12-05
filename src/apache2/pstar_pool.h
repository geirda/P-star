/*
   This file is part of P-star (http://www.p-star.org), and
   is licenced to you under the Apache Licence, Version 2.0.
   You should have received a copy of the licence with this
   software, but it can also be obtained here:

      http://www.apache.org/licenses/LICENSE-2.0
   
   - Copyright MMXIII Atle Solbakken
      http://www.p-star.org
      atle@goliathdns.no 
   
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "httpd.h"

#include "../libwpl/program.h"
#include "../libwpl/exception.h"

#include <string>
#include <map>

struct pstar_file {
	private:
	int mtime;
	wpl_program program;

	public:
	bool check_modified (int mtime) {
		return (this->mtime != mtime ? false : true);
	}
	pstar_file (const char *filename, int mtime);
};

class pstar_pool {
	private:	
	map<string, pstar_file> files;

	int handle_file (request_rec *r, const char *filename, int mtime);

	public:
	apr_status_t handle_request(request_rec *r);
};