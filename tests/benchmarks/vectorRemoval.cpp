#include <celero/Celero.h>

#include <vector>
#include <random>
#include <limits>
#include <algorithm>
#include <iterator>
#include <experimental/algorithm>
#include <unordered_map>
#include <unordered_set>

int destinationSizes[] = {10, 100, 1000, 10000, 20000, 200000};
double sourceFractions[] = {0.01, 0.1, 0.2, 0.4, 0.6, 0.8};
uint64_t iterationsPerSample[] = {1000, 1000, 200, 30, 20, 5};



class VectorRemovalFixture : public celero::TestFixture
{
  public:
    VectorRemovalFixture() {}

    virtual std::vector<std::pair<int64_t, uint64_t>> getExperimentValues() const override
    {
        std::vector<std::pair<int64_t, uint64_t>> problemSpace;
        for(int i = 0; i < 36; ++i) 
            problemSpace.push_back(std::make_pair(int64_t(i), iterationsPerSample[i/6]));

        return problemSpace;
    }

    virtual void setUp(int64_t experimentValue) {
        destination.resize(destinationSizes[experimentValue / 6]);
        int sourceSize = destination.size() * sourceFractions[experimentValue % 6];
        if (sourceSize < 1)
            sourceSize = 1;
        source.resize(sourceSize);
    }

    void randomize() { 
        std::mt19937 rd{std::random_device{}()};
        std::uniform_int_distribution<> dist(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());

        std::generate(destination.begin(), destination.end(), [&rd, &dist](){return dist(rd);});
        std::experimental::sample(destination.begin(), destination.end(), source.begin(), source.size(), 
          std::mt19937{std::random_device{}()});
    }

    void cleanup() {
        destination.clear();
        source.clear();
    }

    std::vector<int> destination;
    std::vector<int> source;
};

BASELINE_F(VectorRemoval, Baseline, VectorRemovalFixture, 100, 1000) 
{
    randomize();
    cleanup();
}

BENCHMARK_F(VectorRemoval, OptimizedNonPreservingOrder, VectorRemovalFixture, 100, 1000)
{
    randomize();
    std::unordered_set<int> elems(source.begin(), source.end());

    auto i = destination.begin();
    auto target = destination.end();
    while(i <= target) {
        if(elems.count(*i) > 0)
            std::swap(*i, *(--target));
        else 
            i++;
    }
    destination.erase(target, destination.end());

    cleanup();
}

BENCHMARK_F(VectorRemoval, OriginalQuestion, VectorRemovalFixture, 100, 1000)
{
    if(destination.size() > 10000)
        return;

    randomize();
    for (auto i: source)
        destination.erase(std::remove(destination.begin(), destination.end(), i), destination.end());    
    cleanup();
}

BENCHMARK_F(VectorRemoval, dkgAnswer, VectorRemovalFixture, 100, 1000)
{
    if(destination.size() > 10000)
        return;

    randomize();
    auto last = std::end(destination);
    std::for_each(std::begin(source), std::end(source), [&](const int& val) {
        last = std::remove(std::begin(destination), last, val);
    });
    destination.erase(last, std::end(destination));
    cleanup();
}

BENCHMARK_F(VectorRemoval, Revolver_OcelotAnswer, VectorRemovalFixture, 100, 1000)
{
    randomize();
    std::sort(destination.begin(), destination.end());
    std::sort(source.begin(), source.end());
    std::vector<int> result;
    result.reserve(destination.size());
    std::set_difference(destination.begin(), destination.end(), source.begin(), source.end(), std::back_inserter(result));
    destination.swap(result);
    cleanup();
}

BENCHMARK_F(VectorRemoval, Jarod42Answer, VectorRemovalFixture, 100, 1000)
{
    if(destination.size() > 10000)
        return;

    randomize();
    const auto isInSource = [&](int e) {
        return std::find(source.begin(), source.end(), e) != source.end();
    };
    destination.erase(std::remove_if(destination.begin(), destination.end(), isInSource), destination.end());
    cleanup();
}

BENCHMARK_F(VectorRemoval, MyAnswer, VectorRemovalFixture, 100, 1000)
{
    randomize();
    std::unordered_map<int, int> counts;
    counts.max_load_factor(0.3);
    counts.reserve(destination.size()); 

    for(auto v: destination) {
        counts[v]++;
    }

    for(auto v: source) {
        counts[v]--;
    }

    auto i = destination.begin();
    for(auto k: counts) {
        if(k.second < 1) continue;       
        i = std::fill_n(i, k.second, k.first);
    }
    destination.resize(std::distance(destination.begin(), i));
    cleanup();
}

