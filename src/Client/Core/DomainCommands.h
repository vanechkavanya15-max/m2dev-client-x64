     uint8_t answerIndex{0};
 };
 
struct QuestConfirmCommand {
    uint8_t answer{0};
    uint32_t requestPID{0};
};

 } // namespace Client::Core
