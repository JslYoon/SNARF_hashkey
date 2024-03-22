
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



int main()
{
  //----------------------------------------
  //GENERATING DATA
  //----------------------------------------

  uint64_t N=10'000'000;  
  vector<uint64_t> v_keys(N,0);

  //Key are multiples of 10000. S= {10000, 20000, 30000,...}
  for(uint64_t i=0;i<N;i++)
  {
      v_keys[i]=i*10000;
  }

  //----------------------------------------
  //SNARF CONSTRUCTION
  //----------------------------------------

  //Initializing variables needed for SNARF construction
  //Bits per Key indicates how many bits per key should the snarf instance use
  double bits_per_key=10.00;
  //batch_size indicates what should be the split of the compressed bit array
  //Smaller batch sizes increases the size of the snrf instance while making range queries faster.
  // We recommend using batch_sz ~ 100-200
  int batch_size=100;

  //declare and initialize a snarf instance
  snarf_updatable_gcs<uint64_t> snarf_instance;
  snarf_instance.snarf_init(v_keys,bits_per_key,batch_size);
    
  //get the size of the snarf instance
  int snarf_sz=snarf_instance.return_size();
  cout<<"Bits per key used by SNARF: "<<snarf_sz*8.00/v_keys.size()<<endl;
  



  //----------------------------------------
  //QUERYING SNARF
  //----------------------------------------

  // SNARF suppports 3 main operations
  // Range Query: checks the existence of keys in a range
  // Insert: inserts the key
  // Delete: deletes the key

  uint64_t left_end=15000;
  uint64_t right_end=16000;


  random_device rd; // obtain a random number from hardware
  mt19937 gen(rd()); // seed the generator
  uniform_int_distribution<> distr(1, 15); // define the range

  // Iterate 100k times.
  // For a random key K, we have a range query of [K+1, K+1001], and validate that the key K+1 will highly likely
  // to be matched to K. We count the number of false positive

  // Compare it with a case where given a random key K, we check for [K+2, K+1002], [K+3, K+1003], ... [K+K, K+1000+K]
  // Graph it and show that keys close will match to similar keys
  int fp = 0;
  int tn = 0;
  for (int i=0; i<10000; i++) {
    
    int K = distr(gen) * 1000;
    snarf_instance.insert_key(K);
    if(snarf_instance.range_query(K+1000, K+2000)) {
      fp++;
    } else {
      tn++;
    }
    
  }
  cout << "this is fp " << fp << " and this is tn: " << tn << endl;


  return 0;
}