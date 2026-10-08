     uint32_t currentSp{0};
     uint32_t maxSp{0};
     int64_t currentGold{0};
    int64_t currentCheque{0};
    int64_t currentGaya{0};
     bool isDead{false};
 
     void Reset() noexcept {
         currentSp = 0;
         maxSp = 0;
         currentGold = 0;
        currentCheque = 0;
        currentGaya = 0;
         isDead = false;
     }
 };
