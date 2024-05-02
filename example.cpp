#include<iostream>
#include<algorithm>
#include<cmath>
#include <fstream>
#include <chrono>
#include <set>
#include <ctime>
#include <cstring>
#include <random>
#include <limits>

using namespace std;
using namespace std::chrono;

#include "include/snarf_hash.cpp"

// To get normal distribution
vector<uint64_t> get_normal_distribution(uint64_t N, double mean, double stddev, uint64_t range_min, uint64_t range_max) {

    std::vector<uint64_t> results;
    results.reserve(N); 

    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> dist(mean, stddev);

    for (size_t i = 0; i < N; ++i) {
        double number;
        do {
            number = dist(gen);
            number = (number - mean) / (4 * stddev) * (range_max - range_min) + (range_min + range_max) / 2.0;
        } while (number < range_min || number > range_max); // Repeat if the number is outside the range

        results.push_back(static_cast<uint64_t>(number));
    }
    return results;
}


// To get uniform distribution
vector<uint64_t> get_uniform_distribution(uint64_t N, uint64_t range_min, uint64_t range_max) {
    vector<uint64_t> v_keys(N, 0);
    random_device rd;
    mt19937_64 gen(rd()); 
    uniform_int_distribution<uint64_t> dist(range_min, range_max);

    for (uint64_t i = 0; i < N; ++i) {
        v_keys[i] = dist(gen);
    }

    return v_keys;
}

// To get exponential distribution
vector<uint64_t> get_exponential_distribution(uint64_t N, double lambda, uint64_t range_min, uint64_t range_max) {
    vector<uint64_t> v_keys(N, 0);
    random_device rd;
    mt19937 gen(rd()); 
    exponential_distribution<> dist(lambda);

    double max_exp_value = log(range_max) / lambda;
    
    for (uint64_t i = 0; i < N; ++i) {
        double exp_value = dist(gen);
        
        
        exp_value = min(max_exp_value, exp_value); 
        double scale = exp_value / max_exp_value;
        v_keys[i] = range_min + static_cast<uint64_t>((range_max - range_min) * scale);
    }
    return v_keys;
}

// Function to find if a query performs a false positive or not; it checks if a value exists within a certain range in source_vec
bool find_key_in(const vector<uint64_t>& source_vec, uint64_t left_end, uint64_t right_end) {
    auto lower = lower_bound(source_vec.begin(), source_vec.end(), left_end);

    if (lower != source_vec.end() && *lower <= right_end) {
        return true; // Found a value within the range
    }

    return false; // No values found within the range
}



// Function to test snarf
void test_snarf(double bits_per_key, uint64_t batch_size, string key_distribution, string query_distribution, 
                  uint64_t test_num, uint64_t N, bool special, string query_option, uint64_t num_hash_bits) {

  //----------------------------------------
  //GENERATING DATA
  //----------------------------------------

  vector<uint64_t> v_keys;

  if (key_distribution == "uniform") { // uniform distribution
    v_keys = get_uniform_distribution(N, 0, static_cast<uint64_t>(pow(2, 50))-1);

  } else if (key_distribution == "normal") { // normal distribution
    v_keys = get_normal_distribution(N, 100.0, 20.0, 0, static_cast<uint64_t>(pow(2, 50))-1);
  }

  // Sort v_keys for faster verification of false positiviry
  vector<uint64_t> sorted_v_keys = v_keys;
  sort(sorted_v_keys.begin(), sorted_v_keys.end());


  //----------------------------------------
  //SNARF CONSTRUCTION
  //----------------------------------------
  
  //declare and initialize a snarf instance
  snarf_updatable_gcs_hash<uint64_t> snarf_instance;
  // snarf_updatable_gcs_hash<uint64_t> snarf_instance;
  snarf_instance.snarf_init(v_keys,bits_per_key,batch_size, num_hash_bits);
    
  //get the size of the snarf instance
  int snarf_sz=snarf_instance.return_size();
  cout<<"Bits per key used by SNARF: "<<snarf_sz*8.00/v_keys.size()<<endl;
  
  //----------------------------------------
  //BUILDING WORKLOAD
  //----------------------------------------

  vector<uint64_t> test_queries;
  if(!special){
    if (query_distribution == "uniform") { // uniform distribution
      test_queries = get_uniform_distribution(N, 0, static_cast<uint64_t>(pow(2, 50))-1);

    } else if (query_distribution == "normal") { // normal distribution
      test_queries = get_normal_distribution(N, 100.0, 20.0, 0, static_cast<uint64_t>(pow(2, 50))-1);

    } else if (query_distribution == "exponential") { // exponential distribution
      test_queries = get_exponential_distribution(N, 10.0, 0, static_cast<uint64_t>(pow(2, 50))-1);
    }
  }

  //----------------------------------------
  //QUERYING SNARF
  //----------------------------------------

  vector<uint64_t> rq_ranges({0, 16, 64, 256});
  uint64_t fp;
  uint64_t tn;
  uint64_t tp;
  double all_rate = 0;
  
  if(!special && (query_option == "all")) {
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < rq_ranges.size(); i++) {
      fp = 0;
      tn = 0;
      tp = 0;
      for (int j = 0; j < test_queries.size(); j++) {
        uint64_t lower_bound = test_queries[j];
        uint64_t upper_bound = lower_bound + rq_ranges[i];
        if(snarf_instance.range_query(lower_bound, upper_bound)) {
          if(find_key_in(sorted_v_keys, lower_bound, upper_bound)) {
            tp++;
          } else {
            fp++;
          }
        } else {
          tn++;
        }
      }    
      double rate = static_cast<double>(fp) / (fp + tn);
      all_rate += rate;
    }
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    all_rate = all_rate / rq_ranges.size();
    cout << "    The false positive rate overall for mixed range query " << key_distribution << " keys and " << query_distribution << " is " << all_rate <<
          " and it took " << duration.count() << " milliseconds" << endl;
  }

  //----------------------------------------
  //SNARF WITH kEY K WE QUERY FROM K+(TEST_NUM)
  //----------------------------------------
  uint64_t TEST_NUM = test_num;
  all_rate = 0;
  auto start = std::chrono::high_resolution_clock::now();
  for(int i = 0; i < rq_ranges.size(); i++) {
    fp = 0;
    tn = 0;
    tp = 0;
    for(int j = 0; j < sorted_v_keys.size(); j++) {

      uint64_t lower_bound;
      uint64_t upper_bound;
      if(!special) {
        lower_bound = sorted_v_keys[j] + TEST_NUM;
        upper_bound = lower_bound + rq_ranges[i];
      } else {
        upper_bound = sorted_v_keys[j] - TEST_NUM;
        lower_bound = upper_bound - rq_ranges[i];
      }
      if(snarf_instance.range_query(lower_bound, upper_bound)) {

        if(find_key_in(sorted_v_keys, lower_bound, upper_bound)) {
          tp++;
        } else {
          fp++;
        }

      } else {
          if(find_key_in(sorted_v_keys, lower_bound, upper_bound)) {
        } 
        tn++;
      }
    }
    double rate = static_cast<double>(fp) / (fp + tn);
    all_rate += rate;
  }
  auto stop = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
  all_rate = all_rate / rq_ranges.size();
  cout << "    The false positive rate for close-K " << test_num << " queries is " << all_rate <<  " and it took " << duration.count() << " milliseconds" << endl;



}


// Helper function for interface display
template<typename T>
uint64_t display_select_vec(vector<T>& vec) {
  uint64_t num = 0;
  bool is_valid = false;
  for(int i = 1; i <= vec.size(); i++) { // display vec
    cout << i << ". " << vec[i-1] << endl;
  }

  while(!is_valid) { // input loop
    cout << "Choose an option (1 to " << vec.size() << "): ";  
    if (cin >> num && num > 0 && num <= vec.size()) { 
      is_valid = true; 
    } else {
      cout << "Invalid input." << endl;
      cin.clear(); 
      cin.ignore(numeric_limits<streamsize>::max(), '\n'); 

    }
  }
  return num;
}

// Until valid number
uint64_t until_number_input(uint64_t min, uint64_t max) {
  
  uint64_t number;
    std::cout << "Enter a number between " << min << " and " << max << ": ";
    while (true) {
        if (std::cin >> number) {
            if (number >= min && number <= max) {
                break;  
            } else {
                std::cout << "Please enter a number within the range " << min << " to " << max << ": ";
            }
        } else {
            cout << "Invalid input." << endl;
            cin.clear();  
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
        }
    }
    return number;
}
        




int main() {  

  // Testable options
  vector<uint64_t> bits_per_keys({6, 8, 10, 12, 14, 16, 18});
  vector<string> key_dists({"normal", "uniform"});
  vector<string> query_dists({"normal", "uniform", "exponential"});
  vector<string> query_options({"all", "close-K"});
  vector<string> interface_options({"Start test", "Choose key distribution", "Choose query distribution", "Choose bits per key", 
                                      "Choose K, K+n", "Choose number of tests", "Change query options", "Special Case: K-n, K-1", 
                                        "Change bits per keys allocated to hashing (*)", "Exit test"});
  uint64_t num_hash_bits = 6;

  string key_dist = "normal";
  string query_dist = "normal";
  string query_option = "all";
  uint64_t bits_per_key = 8;
  uint64_t test_num = 1;  
  uint64_t N=10'000'000;  

 
  cout << "Welcome to SNARF test!" << endl;
  while(true) {

    cout << endl << "----------------------------------------------" << endl
      << "Current options" << endl
      << "  Number of tests: " << N << endl
      << "  Key distribution: " << key_dist << endl
      << "  Query distribution: " << query_dist << endl
      << "  Bits per key: " << bits_per_key << endl
      << "  Testing for K, K+" << test_num << endl
      << "  Query option testing for " << query_option << endl
      << "  Hashing memory allocated: " << num_hash_bits << " bits" << endl
      << "----------------------------------------------" << endl << endl;

    switch(display_select_vec(interface_options)) {
      case 1: // Start test
        cout << endl;
        // string s = snarf_options[display_select_vec(snarf_options)];
        test_snarf(bits_per_key, 100.0,  key_dist,query_dist, test_num, N, false, query_option, num_hash_bits);
        break;

      case 2: // Choose key distribution
        key_dist = key_dists[display_select_vec(key_dists)-1];
        break;

      case 3: // Choose query distribution
        query_dist = query_dists[display_select_vec(query_dists)-1];
        break;

      case 4: // Choose bits per key
        bits_per_key = bits_per_keys[display_select_vec(bits_per_keys)-1];
        break;
      
      case 5: // Choose K, K+n
        test_num = until_number_input(1, N);
        break;

      case 6: // Choose N
        N = until_number_input(100000, 100'000'000);
        break;

      case 7: // Change query option
        query_option = query_options[display_select_vec(query_options)-1];
        break;

      case 8: // K-n, K-1 case
        // string s = snarf_options[display_select_vec(snarf_options)];
        test_snarf(bits_per_key, 100.0,  key_dist,query_dist, test_num, N, true, "all", num_hash_bits);
        break;

      case 9: // change bits per hashing
        num_hash_bits = until_number_input(0,32);
        break;

      case 10: // Exit
        cout << "Goodbye!" << endl;
        return 0;

    }
  }
  
  return 0;
}
