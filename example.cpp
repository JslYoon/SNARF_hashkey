#include<iostream>
#include<algorithm>
#include<cmath>
#include <random>
#include <fstream>
#include <chrono>
#include <set>
#include <ctime>
#include <cstring>
#include <random>
using namespace std;
using namespace std::chrono;
#include "include/snarf.cpp"

// To get normal distribution
vector<uint64_t> get_normal_distribution(uint64_t N, double mean, double stddev, uint64_t range_min, uint64_t range_max) {
    std::vector<uint64_t> v_keys(N, 0);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> dist(mean, stddev);

    for (uint64_t i = 0; i < N; i++) {

        double number = dist(gen);
        v_keys[i] = std::min(std::max(range_min, static_cast<uint64_t>((number - mean) / stddev * (range_max - range_min) + (range_max + range_min) / 2)), range_max);
    }

    return v_keys;
}


// To get uniform distribution
vector<uint64_t> get_uniform_distribution(uint64_t N, uint64_t range_min, uint64_t range_max) {
    std::vector<uint64_t> v_keys(N, 0);
    std::random_device rd;
    std::mt19937_64 gen(rd()); 
    std::uniform_int_distribution<uint64_t> dist(range_min, range_max);

    for (uint64_t i = 0; i < N; ++i) {
        v_keys[i] = dist(gen);
    }
    return v_keys;
}

// To get exponential distribution
std::vector<uint64_t> get_exponential_distribution(uint64_t N, double lambda, uint64_t range_min, uint64_t range_max) {
    std::vector<uint64_t> v_keys(N, 0);
    std::random_device rd;
    std::mt19937 gen(rd()); 
    std::exponential_distribution<> dist(lambda);

    double max_exp_value = std::log(range_max) / lambda;
    
    for (uint64_t i = 0; i < N; ++i) {
        double exp_value = dist(gen);
        
        
        exp_value = std::min(max_exp_value, exp_value); 
        double scale = exp_value / max_exp_value;
        v_keys[i] = range_min + static_cast<uint64_t>((range_max - range_min) * scale);
    }
    return v_keys;
}

// Function to find if a query performs a false positive or not; it checks if a value exists within a certain range in source_vec
bool find_key_in(const std::vector<uint64_t>& source_vec, uint64_t left_end, uint64_t right_end) {
    auto lower = std::lower_bound(source_vec.begin(), source_vec.end(), left_end);

    if (lower != source_vec.end() && *lower <= right_end) {
        return true; // Found a value within the range
    }

    return false; // No values found within the range
}

// Function to test snarf
void test_snarf(double bits_per_key, uint64_t batch_size, string key_distribution, string query_distribution) {
  //----------------------------------------
  //GENERATING DATA
  //----------------------------------------

  uint64_t N=100'000'000;  
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
  snarf_updatable_gcs<uint64_t> snarf_instance;
  snarf_instance.snarf_init(v_keys,bits_per_key,batch_size);
    
  //get the size of the snarf instance
  int snarf_sz=snarf_instance.return_size();
  cout<<"Bits per key used by SNARF: "<<snarf_sz*8.00/v_keys.size()<<endl;
  

  //----------------------------------------
  //BUILDING WORKLOAD
  //----------------------------------------

  vector<uint64_t> test_queries;
  if (query_distribution == "uniform") { // uniform distribution
    test_queries = get_uniform_distribution(N, 0, static_cast<uint64_t>(pow(2, 50))-1);

  } else if (query_distribution == "normal") { // normal distribution
    test_queries = get_normal_distribution(N, 100.0, 20.0, 0, static_cast<uint64_t>(pow(2, 50))-1);

  } else if (query_distribution == "exponential") { // exponential distribution
    test_queries = get_exponential_distribution(N, 10.0, 0, static_cast<uint64_t>(pow(2, 50))-1);
  }

  //----------------------------------------
  //QUERYING SNARF
  //----------------------------------------

  // SNARF suppports 3 main operations
  // Range Query: checks the existence of keys in a range
  // Insert: inserts the key
  // Delete: deletes the key

  vector<uint64_t> rq_ranges({0, 16, 64, 256});
  uint64_t fp = 0;
  uint64_t tn = 0;
  uint64_t tp = 0;
  for(int j = 0; j < rq_ranges.size(); j++) {
    for (int i = 0; i < test_queries.size(); i++) {
      uint64_t lower_bound = test_queries[i];
      uint64_t upper_bound = test_queries[i] + rq_ranges[j];
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
    cout << "this is fp " << fp << " and this is tn: " << tn << " and this is tp " << tp << " for rq_range of " << j << endl;
  }

  


}





int main() {

  test_snarf(10.00, 100, "normal", "normal");

  return 0;
}
