#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <string_view>
#include <chrono>
#include <optional>
#include <span>
#include <functional>
#include <expected>

#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "Core/EventBus.h"

namespace UserInterface::Domain {

/**
 * @brief Event emitted when the trade history is updated.
 */
struct TradeHistoryUpdateEvent : public Core::IEvent {
    size_t newTotalRecords; ///< The new total number of records

    /**
     * @brief Constructs the trade history update event.
     * @param totalRecords Total records currently in history.
     */
    explicit TradeHistoryUpdateEvent(size_t totalRecords) : newTotalRecords(totalRecords) {}
};

/**
 * @brief Type of the trade transaction.
 */
enum class TradeType : uint8_t {
    Exchange,   ///< Direct player-to-player trade
    Shop        ///< Transaction via NPC or player private shop
};

/**
 * @brief Represents a single item transferred during a trade.
 */
struct TradeItem {
    EterBase::ItemVnum vnum;    ///< Virtual number of the item
    uint32_t count;             ///< Quantity of the item
};

/**
 * @brief Represents a single trade or shop transaction record.
 */
struct TradeRecord {
    TradeType type;                                         ///< The type of the trade (Exchange or Shop)
    std::string partnerName;                                ///< Name of the player or NPC involved in the trade
    std::vector<TradeItem> itemsGiven;                      ///< Items given by the local player
    std::vector<TradeItem> itemsReceived;                   ///< Items received by the local player
    uint32_t goldGiven;                                     ///< Amount of gold given by the local player
    uint32_t goldReceived;                                  ///< Amount of gold received by the local player
    std::chrono::system_clock::time_point timestamp;        ///< When the transaction occurred
};

/**
 * @brief Model managing the history of completed trade and shop transactions.
 * 
 * Provides an event-driven, decoupled way to store and retrieve transaction logs.
 */
class TradeHistoryModel {
public:
    /**
     * @brief Default constructor for TradeHistoryModel.
     */
    TradeHistoryModel() = default;

    /**
     * @brief Destructor.
     */
    ~TradeHistoryModel() = default;

    /**
     * @brief Adds a new trade record to the history and publishes an update event.
     * @param record The trade transaction record to add.
     * @return Result containing void on success, or an error description.
     */
    std::expected<void, EterBase::EntityError> AddRecord(TradeRecord record) {
        history_.push_back(std::move(record));
        
        EterBase::ModernLogger::Info("TradeHistoryModel: Added new record. Partner: {}, Type: {}", 
                                     history_.back().partnerName, 
                                     static_cast<int>(history_.back().type));

        Core::EventBus::GetInstance().Publish(TradeHistoryUpdateEvent{history_.size()});

        return {};
    }

    /**
     * @brief Retrieves the entire trade history.
     * @return A read-only span of the trade records.
     */
    [[nodiscard]] std::span<const TradeRecord> GetHistory() const noexcept {
        return history_;
    }

    /**
     * @brief Retrieves a specific trade record by its index safely.
     * @param index The zero-based index of the record.
     * @return Result containing a reference to the record if found, or an EntityError.
     */
    [[nodiscard]] EterBase::Result<std::reference_wrapper<const TradeRecord>, EterBase::EntityError> GetRecord(size_t index) const noexcept {
        if (index >= history_.size()) {
            return std::unexpected(EterBase::EntityError::OutOfRange);
        }
        return std::cref(history_[index]);
    }

    /**
     * @brief Retrieves the partner name for a specific record using monadic optional/expected operations.
     * @param index The zero-based index of the record.
     * @return Result containing the partner name, or an EntityError.
     */
    [[nodiscard]] EterBase::Result<std::string_view, EterBase::EntityError> GetPartnerName(size_t index) const noexcept {
        return GetRecord(index).transform([](auto recordRef) -> std::string_view {
            return recordRef.get().partnerName;
        });
    }

    /**
     * @brief Clears the entire trade history and publishes an update event.
     * @return Result containing void on success.
     */
    std::expected<void, EterBase::EntityError> ClearHistory() {
        history_.clear();
        
        EterBase::ModernLogger::Info("TradeHistoryModel: History cleared.");
        
        Core::EventBus::GetInstance().Publish(TradeHistoryUpdateEvent{0});
        
        return {};
    }

private:
    std::vector<TradeRecord> history_; ///< Internal storage for trade records
};

} // namespace UserInterface::Domain
