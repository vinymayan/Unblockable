#pragma once
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

namespace Tracking
{
    inline std::uint32_t UnblockableHitsSTM = 0;

    // --- SERIALIZAÇÃO (SKSE Cosave) ---
    namespace Serialization
    {
        constexpr std::uint32_t kSerializationID = 'VIN2'; // Identificador principal do Mod
        constexpr std::uint32_t kDataVersion = 1;

        // Sub-ID para identificar o tipo de dado internamente
        constexpr std::uint32_t kUnblockableHitsRecord = 'UBKH';

        inline bool hasLoadedData = false;

        static void SaveCallback(SKSE::SerializationInterface* a_intfc)
        {
            auto player = RE::PlayerCharacter::GetSingleton();

            // Salva a contagem de Unblockable Hits recebidos pelo jogador
            if (a_intfc->OpenRecord(kUnblockableHitsRecord, kDataVersion)) {
                a_intfc->WriteRecordData(UnblockableHitsSTM);
                if (player) {
                    player->SetGraphVariableInt("UnblockableHitsSTM", static_cast<int>(UnblockableHitsSTM));
                }
            }
            else {
                SKSE::log::error("Falha ao abrir record de Unblockable Hits!");
            }
        }

        static void LoadCallback(SKSE::SerializationInterface* a_intfc)
        {
            std::uint32_t type;
            std::uint32_t version;
            std::uint32_t length;

            hasLoadedData = false;
            auto player = RE::PlayerCharacter::GetSingleton();

            while (a_intfc->GetNextRecordInfo(type, version, length)) {
                switch (type) {
                case kUnblockableHitsRecord:
                    a_intfc->ReadRecordData(UnblockableHitsSTM);
                    if (player) {
                        player->SetGraphVariableInt("UnblockableHitsSTM", static_cast<int>(UnblockableHitsSTM));
                    }
                    hasLoadedData = true;
                    break;

                default:
                    SKSE::log::warn("Sub-record desconhecido encontrado: {:X}", type);
                    break;
                }
            }
            SKSE::log::info("Dados carregados - Unblockable Hits: {}", UnblockableHitsSTM);
        }

        static void RevertCallback(SKSE::SerializationInterface*)
        {
            auto player = RE::PlayerCharacter::GetSingleton();
            UnblockableHitsSTM = 0;
            if (player) {
                player->SetGraphVariableInt("UnblockableHitsSTM", static_cast<int>(UnblockableHitsSTM));
            }
            hasLoadedData = false;
        }
    }
}