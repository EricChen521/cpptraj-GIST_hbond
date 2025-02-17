#include "ClusterSieve.h"
#include "Random.h"

// CONSTRUCTOR
ClusterSieve::ClusterSieve() : type_(NONE), sieve_(1), actualNframes_(0) {}

void ClusterSieve::Clear() {
  frameToIdx_.clear();
  type_ = NONE;
  sieve_ = 1;
  actualNframes_ = 0;
}

inline void ClusterSieve::DetermineTypeFromSieve( int sieveIn ) {
  sieve_ = sieveIn;
  // Determine sieve type from sieve value.
  if (sieve_ < -1)
    type_ = RANDOM;
  else if (sieve_ < 2) {
    type_ = NONE;
    sieve_ = 1;
  } else
    type_ = REGULAR;
}

// ClusterSieve::SetSieve()
int ClusterSieve::SetSieve(int sieveIn, size_t maxFrames, int iseed) {
  if (maxFrames < 1) return 1;
  DetermineTypeFromSieve( sieveIn );
  frameToIdx_.clear();
  if (type_ == NONE) 
  { // No sieving; frame == index
    frameToIdx_.reserve( maxFrames );
    for (unsigned int i = 0; i < maxFrames; i++)
      frameToIdx_.push_back( i );
    actualNframes_ = (int)maxFrames;
  }
  else if (type_ == REGULAR)
  { // Regular sieveing; index = frame / sieve
    frameToIdx_.assign( maxFrames, -1 );
    int idx = 0;
    for (unsigned int i = 0; i < maxFrames; i += sieve_)
      frameToIdx_[i] = idx++;
    actualNframes_ = idx;
  }
  else if (type_ == RANDOM)
  { // Random sieving; maxframes / sieve random indices
    frameToIdx_.assign( maxFrames, -1 );
    unsigned int maxidx = maxFrames - 1;
    Random_Number random;
    random.rn_set( iseed );
    for (unsigned int i = 0; i < maxFrames; i -= sieve_)
    {
      bool frame_generated = false;
      // Pick until we pick a frame that has not yet been selected.
      while (!frame_generated) {
        unsigned int iframe = random.rn_num_interval(0, maxidx);
        if (frameToIdx_[iframe] == -1) {
          frameToIdx_[iframe] = 1;
          frame_generated = true;
        }
      }
    }
    // Put indices in order
    int idx = 0;
    for (unsigned int i = 0; i < maxFrames; i++)
      if (frameToIdx_[i] == 1)
        frameToIdx_[i] = idx++;
    actualNframes_ = idx;
  }
  MakeIdxToFrame();
  return 0;
}

// ClusterSieve::SetSieve()
int ClusterSieve::SetSieve(int sieveIn, std::vector<char> const& sieveStatus) {
  DetermineTypeFromSieve( sieveIn );
  if (sieveStatus.empty()) return 1;
  frameToIdx_.clear();
  frameToIdx_.assign( sieveStatus.size(), -1 );
  unsigned int idx = 0;
  for (unsigned int frame = 0; frame < sieveStatus.size(); ++frame)
  {
    if ( sieveStatus[frame] == 'F' )
      frameToIdx_[frame] = idx++;
  }
  actualNframes_ = (int)idx;
  MakeIdxToFrame();
  return 0;
}

// ClusterSieve::MakeIdxToFrame()
void ClusterSieve::MakeIdxToFrame() {
  idxToFrame_.clear();
  idxToFrame_.reserve( actualNframes_ );
  for (unsigned int frame = 0; frame != frameToIdx_.size(); frame++)
  {
    if (frameToIdx_[frame] != -1)
      idxToFrame_.push_back( frame );
  }
}

size_t ClusterSieve::DataSize() const {
  return ( sizeof(type_) + sizeof(int) +
           (frameToIdx_.capacity()*sizeof(int) + sizeof(frameToIdx_)) +
           (idxToFrame_.capacity()*sizeof(int) + sizeof(idxToFrame_)) );
}
