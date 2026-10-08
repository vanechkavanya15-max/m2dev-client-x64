     void Guild::SetMark(const std::string& mark) {
         m_mark = mark;
     }
    
    uint32_t Guild::GetBank() const {
        return m_bank;
    }
    
    void Guild::SetBank(uint32_t bank) {
        m_bank = bank;
    }
    
    void Guild::SetExp(uint8_t level, uint32_t exp) {
        m_level = level;
        m_exp = exp;
    }
    
    std::pair<uint8_t, uint32_t> Guild::GetExp() const {
        return {m_level, m_exp};
    }
 
     EterBase::VoidResult<> Guild::AddMember(const GuildMember& member) {
         if (std::ranges::find_if(m_members, [&](const auto& m) { return m.vid == member.vid; }) != m_members.end()) {
