class Potato {
private:
    int num_hops;
    int trace[512];
    size_t size;

public:
    Potato() : num_hops(0), trace(), size(0) {}

    int getNumHops() const {
        return num_hops;
    }

    void setNumHops(int _num_hops) {
        num_hops = _num_hops;
    }

    void decrNumHops() {
        num_hops--;
    }

    void incrSize() {
        size++;
    }

    void addToTrace(int player_id) {
        trace[size] = player_id;
    }

    void printTrace() {
        for (size_t i = 0; i < size; i++) {
            std::cout << trace[i];
            if (i + 1 != size)
                std::cout << ",";
        }
        std::cout << std::endl;
    }
};
