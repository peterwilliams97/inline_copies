#include <memory.h>
#include <assert.h>

#include <list>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <iostream>
#include "BinString.h"
#include "Timer.h"
#include "boyer_moore.h"

/*
 * Rough plan for faster than disk-speed inline copies detection
 *
 * Given an N page spool file of S_F bytes
 *

 Worst case behaviours

 2 copies because boyer moore needs to build tables based on search pattern that is half length of fil
 copies that differ only at end 
  *
 */
using namespace std;

#define TEST_RAW_BOYER_MORE 1
#define COPY_HEADER_SIZE 0

static const double MIN_TEST_DURATION = 1.0;
static const bool VERBOSE = false;

void show_data(const byte *data, size_t len, const char *desc) {
    if (VERBOSE) {
        cout << (int)len << " bytes: "; 
        for (int i = 0; i < min(20, (int)len); i++) {
            char s[10];
            sprintf(s, "%02x,", data[i]);
            cout << s;
        }
        cout << " " << desc << endl;
        cout << "......................." << endl;
    }
}

template <class T>
void show_vector(const vector<T> vec, const char *desc) {
    cout << desc << " = [";
    for (unsigned int i = 0; i < vec.size(); i++) {
        cout << " " << vec[i];
    }
    cout << "]" << endl;
}

const vector<int> get_factors(int number) {
    list <int> factors;

    int n = number;
    for (int i = 2; i <= number; i++) {
        if (n % i == 0) {
            factors.push_front(i);
        }
    }
    return vector<int>(factors.begin(), factors.end());
}    

/*
 * Calculate copy size S_C = S_F/Mi
 * Set pattern size S_H = S_C/2 + 2 (just bigger then half a copy) 			
 * Take pattern Pat S_P/4 .. S_P/4 + S_H  i.e  in middle of 1st copy			 
 * Search remainder of input stream.
 *      If there is not at least one match in each candidate copy offset S_C*i .. S_C*(i+1) for i = 0..N-1
 *	then there cannot be inline copies => quit 
 * In this helper we find all repeats of the smallest single copy size
 * Later on we work up to bigger copy sizes
 * We start here because larger copies cannot match n times unless smaller copies do as well
 */


/*
 * Return offsets of all repeats of a pattern from the middie of the first copy in
 *  input
 */
vector<size_t> find_repeats(const BinString &input, int num_copies, size_t *repeat_len) {
     
    size_t copy_size = input.get_len()/num_copies;
    size_t pattern_size = copy_size - COPY_HEADER_SIZE;
    size_t pattern_ofs = (copy_size- pattern_size)/2;

    cout << " find_repeats: " 
        << "num_copies=" << num_copies 
        << ",copy_size=" << (int)copy_size 
        << ",pattern_size=" << (int)pattern_size 
        << ",pattern_ofs=" << (int)pattern_ofs 
        << endl;
    
    const byte *end = input.get_data() + input.get_len();
    const byte *pat = input.get_data() + pattern_ofs;
    const byte *text = pat + pattern_size;
    size_t textlen = end - text;

    vector<const byte *> pointers = boyer_moore_all(text, textlen, pat, pattern_size, copy_size);

    list<size_t> offsets;
    offsets.push_back(pat - input.get_data());
    for (unsigned int i = 0; i < pointers.size(); i++) {
        offsets.push_back(pointers[i] - input.get_data());
    } 

    cout << "   num matches=" << (int)offsets.size() << endl;

    *repeat_len = copy_size;
    return vector<size_t>(offsets.begin(), offsets.end());
}

vector<int> filter_candidates(vector<int> numcopies_candidates, vector<size_t> repeats) {
        
    // Simple run_test. Filter out num copies < num repeats
    list<int> remaining_candidates;
    for (unsigned int i = 1; i < numcopies_candidates.size(); i++) {
        int num_copies = numcopies_candidates[i];
        if ((int)repeats.size() >= num_copies) {
            remaining_candidates.push_back(num_copies);
        }   
    } 
    numcopies_candidates = vector<int>(remaining_candidates.begin(), remaining_candidates.end());
    //show_vector(numcopies_candidates, "  filter_candidates");

#if 0
    // Check for plausible locations
    // There must be at least one repeated block within each copy boundary
    // ALREADY CHECKED
    remaining_candidates = list<int>();
    for (vector<int>::iterator it = numcopies_candidates.begin(); it != numcopies_candidates.end(); it++) {
        int num_copies = *it;
        size_t copy_size = input.get_len()/num_copies;
        bool match = true;
        int j = 0;
        for (int i = 0; i < num_copies-1; i++) {
            if (!(i * copy_size <= all_repeats[j] && all_repeats[j] <= (i+1) * copy_size)) {
                match = false;
                break;
            }       
        }
        if (match) {
            remaining_candidates.push_back(num_copies);
        }
    } 
    numcopies_candidates = vector<int>(remaining_candidates.begin(), remaining_candidates.end());;
#endif  
    return numcopies_candidates;
}

vector<int> filter_candidates2(vector<int> numcopies_candidates, list<vector<size_t> > all_repeats) {
    set<int> all_candidates;
    for (list<vector<size_t> >::iterator it = all_repeats.begin(); it != all_repeats.end(); it++) {
        vector<int> candidates = filter_candidates(numcopies_candidates, *it);
        for (unsigned int i = 0; i < candidates.size(); i++) {
            all_candidates.insert(candidates[i]);
        }
    } 
    //The set is sorted. We want reverse order so use r iterations
    return vector<int>(all_candidates.rbegin(), all_candidates.rend());
}


int find_num_copies(const BinString &input, int num_pages, vector<int> numcopies_candidates) {
 
    list<vector<size_t> > all_repeats;

    while (true) {

        int num_copies = numcopies_candidates[0];
        size_t repeat_len;
        vector<size_t> repeats = find_repeats(input, num_copies, &repeat_len); 
        if ((int)repeats.size() >= num_copies) {
            cout << "---------------" << endl;
            cout << "Found " << num_copies << " copies" << endl;
            for (int i = 0; i < (int)repeats.size(); i++) {
                show_data(input.get_data() + repeats[i], repeat_len, "find");
            }
            return num_copies;
        }

        all_repeats.push_back(repeats);

        numcopies_candidates = filter_candidates2(numcopies_candidates, all_repeats);
        show_vector(numcopies_candidates, "filtered numcopies_candidates");
        if (numcopies_candidates.size() == 0) {
            return -1;
        } 
    }
    
}

int find_copies(const BinString &input, int num_pages) {
    const byte *data = input.get_data();
    size_t num_bytes = input.get_len();

    cout << "num_pages = " << num_pages << endl;

    vector<int> numcopies_candidates = get_factors(num_pages);
    
    show_vector(numcopies_candidates, "numcopies_candidates");
    cout  << "................." << endl;

    return find_num_copies(input, num_pages, numcopies_candidates);
}

#define PRIME_1 15485867
#define PRIME_2 32452843

/*
 * Unit run_test
 */
const BinString *make_copies(int num_copies, size_t copy_size) {
    size_t num_bytes = num_copies * copy_size;
    BinString *bin_string = new BinString(num_bytes);
    byte *data = bin_string->get_data();
    
    unsigned int k = 0;
    for (size_t i = 0; i < copy_size; i++) {
        data[i] = k % 256;
        k = (k + PRIME_1) * PRIME_2; 
    }

    show_data(data, num_bytes, "make_copies");
    for (int n = 1; n < num_copies; n++) {
        memcpy(data + n * copy_size, data, copy_size);
        show_data(data, num_bytes, "updated");
    }
   
    show_data(bin_string->get_data(), bin_string->get_len(), "bin_string");
    return bin_string;
}

BinString make_copies2(int num_pages, int num_copies, size_t copy_size) {
    size_t num_bytes = num_copies * copy_size;
    size_t page_size = num_bytes / num_pages;
    
    assert((int)num_bytes > num_pages);
    assert(num_bytes % num_pages == 0);
      
    byte *data = new byte[num_bytes];
    unsigned int k = 0;
    for (size_t i = 0; i < page_size; i++) {
        data[i] = k % 256;
        k = (k + PRIME_1) * PRIME_2; 
    }

    show_data(data, num_bytes, "make_copies2");
    for (int n = 1; n < num_pages; n++) {
        memcpy(data + n * page_size, data, page_size);
        show_data(data, num_bytes, "updated");
    }

    for (int n = 0; n < num_copies; n++) {
        memcpy(data + (n+1) * copy_size -11, "0123456789", 10);
        //memcpy(data + n * copy_size, "0123456789", 10);
        show_data(data, num_bytes, "updated");
    }

    BinString bin_string = BinString(data, num_bytes);
    delete[] data;
    show_data(bin_string.get_data(), bin_string.get_len(), "bin_string");
    return bin_string;
}

struct Logger {
    FILE* _f;
    Logger(const char *filename) {
        _f = fopen(filename, "wt");
         fprintf(_f, "%5s, %4s, %8s, %4s %6s %6s\n", "num_pages", "num_copies", "copy_size (bytes)", "total_size (MB)", "duration (sec)", "speed (MB/sec)");
    }
    ~Logger() {
        fclose(_f);
    }
    void log(int num_pages, int num_copies, size_t copy_size, double duration) {
        double total_size = (double)num_copies * (double)copy_size /1024.0 /1024.0;
        double speed = duration > 0.0 ? total_size/duration : -1.0;
        fprintf(_f, "%5d, %4d, %8u, %4.1f, %6.2f, %6.3f\n", 
            num_pages, num_copies, copy_size, total_size, duration, speed);
        fflush(_f);
    } 
};

Logger _logger("inline.copies.log");
Timer _timer;

bool run_test(int num_pages, int num_copies, size_t copy_size, double *test_duration) {
   
    assert(num_pages >= num_copies);
    assert(num_pages % num_copies == 0);
    int pages_copy = num_pages / num_copies;
    // Round up copy to next multiple of page per copy
    copy_size = ((copy_size + pages_copy - 1)/pages_copy) * pages_copy;
   // BinString bin_string = make_copies2(num_pages, num_copies, copy_size);
    const BinString *bin_string_ptr = make_copies(num_copies, copy_size);
    const BinString &bin_string = *bin_string_ptr;
  
    show_data(bin_string.get_data(), bin_string.get_len(), "main");
    cout << "---------------" << endl;
    cout << "Input num_pages=" << num_pages 
        << ",num_copies=" << num_copies 
        << ",copy_size=" << (int)copy_size
        << ",file size=" << (double)copy_size * (double)num_copies/1024.0/1024.0 << " MB"
        << endl;
    
    double t0 = _timer.get();
    double  t1; 
    int found_num_copies;
    int num_repeats = 0; 
    do {   
#if TEST_RAW_BOYER_MORE
        size_t patlen = bin_string.get_len()/num_copies;
        const byte *pat = bin_string.get_data();
        const byte *text = bin_string.get_data() + patlen;
        size_t textlen = bin_string.get_len() - patlen; 
        size_t min_gap = (3 * patlen)/4; 
        boyer_moore_all(text, textlen, pat, patlen, min_gap);
        found_num_copies = num_copies;
#else
        found_num_copies = find_copies(bin_string, num_pages);
#endif
        t1 = _timer.get();
        num_repeats++;
    } while (t1 <= t0 + MIN_TEST_DURATION);
   
    *test_duration = (t1 - t0) / (double)num_repeats;
   
    cout << "-----------------------------------------------" << endl;
    cout << "Found " << found_num_copies << " copies" << endl;
    cout << "test took " << *test_duration << " seconds" << endl;
    cout << "==============================================" << endl;
    _logger.log(num_pages, num_copies, copy_size, *test_duration);
    
    bool ok = (found_num_copies == num_copies);
    if (!ok) {
        cerr << "run_test failed: num_pages=" << num_pages
            << ",num_copies=" << num_copies 
            << ",copy_size=" << (int)copy_size << endl;
        cerr << "error!!!" << endl;
    }

    delete bin_string_ptr;
    return ok;

}

int main() {

    double test_duration = -1.0;
    
    run_test(4, 2, 40, &test_duration);
    //return 0;

    int num_pages = 20;   
    int num_copies = 2;
    size_t copy_size = 891;
   
    run_test(num_pages, num_copies, copy_size, &test_duration);

    run_test(40, 20, 50*1000, &test_duration);
    run_test(400, 200, 50*1000, &test_duration);
    run_test(400, 200, 500*1000, &test_duration);
    run_test(4000, 2000, 50*1000, &test_duration);
    run_test(400000, 200000, 5*100, &test_duration);
    run_test(400000, 200000, 5*1000, &test_duration);
    run_test(40, 20, 500*1000, &test_duration);
    run_test(34, 17, 500*1000, &test_duration);
    run_test(51, 17, 500*1000, &test_duration);
    run_test(68, 17, 500*1000, &test_duration);


    // Performance run_test
    num_pages = 2*3*5*7*8;   
//    num_pages = 2*(3*5*7*8+1); 
//    num_pages = 2;
    num_copies = 2;
    copy_size = 100*1000*1000/num_copies;
       
    run_test(num_pages, num_copies, copy_size, &test_duration);
    

    int pages_per_copy = 1;
    int max_pages_per_copy = 100;

    for (copy_size = 99; copy_size < 1000000; copy_size *= 9) {
        for (num_copies = 2; num_copies < 100; num_copies++) {
            num_pages = num_copies * pages_per_copy;
            bool ok = run_test(num_pages, num_copies, copy_size, &test_duration);
            if (!ok) {
                 return 1;
            }

            pages_per_copy += 1;
            if (pages_per_copy > (int)copy_size/10) {
                pages_per_copy = (int)copy_size/10;
            }
            if (pages_per_copy > max_pages_per_copy) {
                pages_per_copy = max_pages_per_copy;
            }
        }
    }
    cout << "************************************************" << endl;
    return 0;
}

