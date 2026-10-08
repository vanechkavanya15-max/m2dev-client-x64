         uint8_t GetRank() const;
         const std::string& GetMark() const;
         void SetMark(const std::string& mark);
        uint32_t GetBank() const;
        void SetBank(uint32_t bank);
        void SetExp(uint8_t level, uint32_t exp);
        std::pair<uint8_t, uint32_t> GetExp() const;
 
         EterBase::VoidResult<> AddMember(const GuildMember& member);
         EterBase::VoidResult<> RemoveMember(EterBase::EntityId vid);
         EterBase::GuildId m_id;
         std::string m_name;
         uint8_t m_rank;
        uint32_t m_bank = 0;
        uint8_t m_level = 0;
        uint32_t m_exp = 0;
         std::string m_mark;
         std::vector<GuildMember> m_members;
     };
