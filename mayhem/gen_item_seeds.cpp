// Seed generator for the item-definitions target (not part of the fuzz build).
//
// The item readers start from a fixed ItemDefinition record (at least kFixedFieldEnd bytes) and
// then follow self-relative pointers to class-tagged blocks and typed arrays. A short or random
// blob is rejected before any of that, so this writes structurally valid definitions laid out
// with the same schema constants the readers use (it includes the reader to see its private
// offsets). Each seed carries the harness's leading op byte.
//
// Rebuild the seeds (from the repo root):
//   clang++ -std=c++20 -ISunrise/src mayhem/gen_item_seeds.cpp \
//     Sunrise/src/middleware/content/packages/tables/item_appearance_reader.cpp \
//     Sunrise/src/middleware/content/packages/tables/definition_index_table.cpp \
//     Sunrise/src/middleware/content/packages/reader/typed_array.cpp \
//     mayhem/harnesses/log_stub.cpp -o /tmp/gen_item_seeds
//   /tmp/gen_item_seeds mayhem/item-definitions/testsuite
#include "middleware/content/packages/tables/item_definition_reader.cpp"

#include <cstdio>
#include <string>
#include <vector>

#include "middleware/content/packages/schema/generated/tag_layout_80802c_0.h"
#include "middleware/content/packages/schema/generated/tag_layout_80809f_0.h"

// Inside the reader's namespace so its private (anonymous-namespace) offsets resolve.
namespace sunrise::middleware::content::packages::tables::items::seedgen {
namespace gen = schema::generated;

constexpr std::uint32_t kMarker = gen::STypedArrayMarker::kClass;

struct Blob {
    std::vector<std::byte> b;
    explicit Blob(std::size_t fixed) : b(fixed) {}
    template <typename T> void put(std::size_t at, T v) {
        if (at + sizeof v > b.size()) b.resize(at + sizeof v);
        std::memcpy(b.data() + at, &v, sizeof v);
    }
    std::size_t alloc(std::size_t n, std::size_t prefix) {
        std::size_t at = (b.size() + prefix + 15) & ~std::size_t{15};
        b.resize(at + n);
        return at;
    }
    void rel(std::size_t field, std::size_t target, std::size_t base) {
        put<std::int64_t>(field, static_cast<std::int64_t>(target) - static_cast<std::int64_t>(base));
    }
    // Plain self-relative block (no class word).
    std::size_t block(std::size_t field, std::size_t size) {
        std::size_t at = alloc(size, 0);
        rel(field, at, field);
        return at;
    }
    // Class-tagged block: u32 class word just before the block.
    std::size_t class_block(std::size_t field, std::uint32_t cls, std::size_t size) {
        std::size_t at = alloc(size, 4);
        put<std::uint32_t>(at - 4, cls);
        rel(field, at, field);
        return at;
    }
    // Typed array: marker word, {u64 count, u32 class, u32 pad}, rows. Descriptor = {u64 count at
    // countAt, i64 relative at ptrAt, relative to ptrAt}. Returns the first row's offset.
    std::size_t array(std::size_t countAt, std::size_t ptrAt, std::uint32_t cls, std::uint64_t n,
                      std::size_t stride) {
        std::size_t header = alloc(16 + n * stride, 4);
        put<std::uint32_t>(header - 4, kMarker);
        put<std::uint64_t>(header, n);
        put<std::uint32_t>(header + 8, cls);
        put<std::uint64_t>(countAt, n);
        rel(ptrAt, header, ptrAt);
        return header + 16;
    }
    std::size_t array(std::size_t desc, std::uint32_t cls, std::uint64_t n, std::size_t stride) {
        return array(desc, desc + reader::layout::kArrayFieldRelative, cls, n, stride);
    }
    // Unlock expression with `n` (opcode, operand) instructions.
    void expression(std::size_t field, std::initializer_list<std::pair<std::uint32_t, std::uint32_t>> ins) {
        std::size_t rows = array(field + kUnlockExpressionCountOffset,
                                 field + kUnlockExpressionPointerOffset,
                                 kInvestmentExpressionRowClass, ins.size(), kUnlockInstructionStride);
        std::size_t i = 0;
        for (auto [op, arg] : ins) {
            put<std::uint32_t>(rows + i * kUnlockInstructionStride + kUnlockInstructionOpcodeOffset, op);
            put<std::uint32_t>(rows + i * kUnlockInstructionStride + kUnlockInstructionOperandOffset, arg);
            ++i;
        }
    }
};

void fixed_fields(Blob& d, std::uint8_t bucket, std::int32_t maxStack) {
    d.put<std::uint8_t>(kBucketIdOffset, bucket);
    d.put<std::int32_t>(kMaxStackSizeOffset, maxStack);
    d.put<std::uint8_t>(kTierOffset, 5);
    d.put<std::uint8_t>(kInstancedOffset, 1);
    d.put<std::uint8_t>(kRecoveryBucketOffset, 34);
    d.put<std::uint32_t>(kStackUniqueLabelOffset, 0x811C9DC5u);
    d.put<std::uint8_t>(kQuietAcquisitionOffset, 1);
    d.put<std::uint8_t>(kToastlessAcquisitionOffset, 0);
    std::size_t action = d.class_block(kActionBlockOffset, kActionBlockClass, sizeof(gen::C80807ad8));
    d.put<std::uint8_t>(action + kActionDeletesOffset, 1);
    d.put<std::uint8_t>(action + kActionConsumesStackOffset, 1);
}

void plug_block(Blob& d) {
    std::size_t p = d.block(kPlugBlockOffset, sizeof(gen::PlugBlock));
    d.put<std::uint32_t>(p + kPlugBlockCategoryOffset, 0x1234u);
    d.put<std::uint16_t>(p + kPlugBlockRollSetOffset, 3);
    d.put<std::uint16_t>(p + kInsertionMaterialRequirementSetIndexOffset, 7);
    d.put<std::uint16_t>(p + kEnabledMaterialRequirementSetIndexOffset, 8);
    std::size_t q = d.array(p + kPlugQuantityExpressionOffset, kInvestmentExpressionRowClass, 1,
                            kUnlockInstructionStride);
    d.put<std::uint32_t>(q + kUnlockInstructionOpcodeOffset, kUnlockReadValueOpcode);
    d.put<std::uint32_t>(q + kUnlockInstructionOperandOffset, 12);
    std::size_t rules = d.array(p + kInsertionRulesOffset, kInsertionRuleClass, 2, kInsertionRuleStride);
    d.expression(rules + kInsertionRuleExpressionOffset, {{kUnlockReadFlagOpcode, 42}});
    d.expression(rules + kInsertionRuleStride + kInsertionRuleExpressionOffset,
                 {{kUnlockReadFlagOpcode, 42}, {kUnlockLiteralOpcode, 1}, {kUnlockAndOpcode, 0}});
    std::size_t linked = d.array(p + kLinkedPlugArrayOffset, kLinkedPlugClass, 1, sizeof(gen::LinkedPlugBlock));
    d.put<std::uint16_t>(linked + kLinkedPlugIndexOffset, 99);
}

void stats_and_appearance(Blob& d) {
    // Stat block: count + self-relative pointer to a typed array of stat rows; perk array.
    std::size_t s = d.block(kStatBlockOffset, sizeof(gen::SItemSandboxPerkBlock));
    std::size_t rows = d.array(s + kStatCountMember, s + kStatDataMember, gen::SItemStatRow::kClass, 2,
                               kStatEntryStride);
    for (int i = 0; i < 2; ++i) {
        d.put<std::uint32_t>(rows + i * kStatEntryStride + kStatEntryRow, 10 + i);
        d.put<std::int32_t>(rows + i * kStatEntryStride + kStatEntryValue, 5 * (i + 1));
    }
    std::size_t perks = d.array(s + offsetof(gen::SItemSandboxPerkBlock, itemPerks), kSandboxPerkClass, 3,
                                sizeof(gen::ItemSandboxPerk));
    d.put<std::uint16_t>(perks, 17);
    d.put<std::uint16_t>(perks + sizeof(gen::ItemSandboxPerk), 0xFFFF);
    d.put<std::uint16_t>(perks + 2 * sizeof(gen::ItemSandboxPerk), 18);
    // Art block: gear art index, art rows, one material-override stage.
    std::size_t a = d.block(kArtBlockOffset, sizeof(gen::ItemTranslationBlock));
    d.put<std::uint16_t>(a + offsetof(gen::ItemTranslationBlock, gearArtIndex), 4);
    std::size_t art = d.array(a + offsetof(gen::ItemTranslationBlock, artRows), kArtRowClass, 2,
                              sizeof(gen::ItemArtRow));
    d.put<std::int8_t>(art, -1);
    d.put<std::uint16_t>(art + offsetof(gen::ItemArtRow, arrangementRow), 6);
    d.put<std::int8_t>(art + sizeof(gen::ItemArtRow), 1);
    d.put<std::uint16_t>(art + sizeof(gen::ItemArtRow) + offsetof(gen::ItemArtRow, arrangementRow), 7);
    std::size_t m = d.array(a + offsetof(gen::ItemTranslationBlock, materialOverrides1), kMaterialOverrideClass,
                            2, sizeof(gen::ItemMaterialOverride));
    d.put<std::int8_t>(m, 2);
    d.put<std::uint16_t>(m + offsetof(gen::ItemMaterialOverride, value), 9);
    d.put<std::int8_t>(m + sizeof(gen::ItemMaterialOverride), kEmptyOverrideKey);
}

void sockets(Blob& d, std::size_t lanes) {
    std::size_t sb = d.alloc(sizeof(gen::TagArray<gen::Unknown>), 0);
    d.rel(kSocketBlockOffset, sb, kSocketBlockOffset);
    std::size_t e = d.array(sb, kOrdinarySocketClass, lanes, kSocketEntryStride);
    for (std::size_t l = 0; l < lanes; ++l) {
        std::size_t at = e + l * kSocketEntryStride;
        d.put<std::uint16_t>(at + kSocketTypeOffset, 20 + l);
        d.put<std::uint16_t>(at + kSocketPlugOffset, 300 + l);
        d.put<std::uint16_t>(at + kReusablePlugSetIndexOffset, l == 0 ? 0 : 0xFFFF);
        d.put<std::uint16_t>(at + kRandomizedPlugSetIndexOffset, l == 0 ? 1 : 0xFFFF);
        if (l == 0) {
            std::size_t pl = d.array(at + kEmbeddedPlugListOffset, gen::PlugSetMember::kClass, 2, kPlugMemberStride);
            d.put<std::uint32_t>(pl + kPlugMemberIndexOffset, 400);
            d.put<std::uint32_t>(pl + kPlugMemberStride + kPlugMemberIndexOffset, 401);
        }
    }
}

// A weapon: equipment slot, socket entry list, sockets, plug, stats, art.
Blob weapon() {
    Blob d(sizeof(gen::ItemDefinition));
    fixed_fields(d, 1, 1);
    plug_block(d);
    stats_and_appearance(d);
    std::size_t sel = d.block(kSocketEntryListBlockOffset, kSocketEntryListBlockSize);
    d.put<std::uint16_t>(sel + kSocketEntryListIndexOffset, 55);
    std::size_t eq = d.block(kEquipmentBlockOffset, sizeof(gen::ItemEquippingBlock));
    d.put<std::int32_t>(eq + kEquipmentSlotOffset, 2);
    sockets(d, 3);
    return d;
}

// A pursuit (bounty): objective block with completion metadata, unlock block with presence flags
// and value slots, value (reward preview) block, lifetime expression.
Blob pursuit() {
    Blob d(sizeof(gen::ItemDefinition));
    fixed_fields(d, state::build_data::items::kPursuitBucketId, 1);
    std::size_t ob = d.class_block(kItemObjectiveBlockOffset, kItemObjectiveBlockClass, sizeof(gen::ItemObjectiveBlock));
    d.put<std::uint8_t>(ob + kSuppressCompletionOffset, 0);
    d.put<std::uint8_t>(ob + kRequireAllObjectivesOffset, 1);
    d.put<std::uint8_t>(ob + kRequireNonOptionalCompletionOffset, 1);
    d.put<std::uint8_t>(ob + kAutomaticCompletionOffset, 0);
    d.put<std::uint16_t>(ob + kCompletionValueOffset, 3);
    d.put<std::uint16_t>(ob + kCompletionContextValueOffset, 4);
    std::size_t objs = d.array(ob + kObjectiveArrayDescriptor, kObjectiveReferenceArrayClass, 2, kObjectiveReferenceStride);
    d.put<std::uint16_t>(objs, 11);
    d.put<std::uint16_t>(objs + kObjectiveReferenceStride, 12);
    std::size_t ub = d.class_block(kItemUnlockBlockOffset, kItemUnlockBlockClass, sizeof(gen::ItemUnlockBlock));
    std::size_t fl = d.array(ub + kItemPresenceFlagDescriptor, kItemPresenceFlagClass, 2, kItemPresenceFlagStride);
    d.put<std::uint16_t>(fl + kItemPresenceFlagSlotOffset, 5);
    d.put<std::uint16_t>(fl + kItemPresenceFlagStride + kItemPresenceFlagSlotOffset, 6);
    std::size_t vs = d.array(ub + kItemValueSlotDescriptor, kItemValueSlotClass, 2, kItemValueSlotStride);
    d.put<std::int16_t>(vs + kItemValueSlotOffset, 1);
    d.put<std::int16_t>(vs + kItemValueSlotStride + kItemValueSlotOffset, 2);
    std::size_t vb = d.class_block(kValueBlockOffset, kValueBlockClass, sizeof(gen::C80807875));
    d.put<std::uint32_t>(vb + kValueRowCountOffset, 1);
    for (std::size_t i = 0; i < kRewardPreviewCapacity; ++i) {
        std::size_t at = vb + kValueRowsOffset + i * kValueRowStride;
        d.put<std::uint32_t>(at, i == 0 ? 77 : kUnavailableRewardReference);
        d.put<std::int32_t>(at + kValueRowQuantityOffset, i == 0 ? 2 : 0);
        d.put<std::uint32_t>(at + kValueRowConditionOffset, kUnavailableRewardReference);
    }
    std::size_t lt = d.array(kLifetimeExpressionField, kInvestmentExpressionRowClass, 1, kUnlockInstructionStride);
    d.put<std::uint32_t>(lt + kUnlockInstructionOpcodeOffset, kUnlockLiteralOpcode);
    d.put<std::uint32_t>(lt + kUnlockInstructionOperandOffset, 86400);
    return d;
}

// Shared plug-set table: two sets, each with member rows whose condition is a flag expression.
Blob plug_set_table() {
    Blob t(sizeof(gen::C80802cb6));
    std::size_t sets = t.array(offsetof(gen::C80802cb6, table), gen::SPlugSetRow::kClass, 2, kPlugSetRowStride);
    for (std::size_t s = 0; s < 2; ++s) {
        std::size_t desc = sets + s * kPlugSetRowStride + kPlugSetMemberDescriptorOffset;
        std::size_t m = t.array(desc, gen::PlugSetMember::kClass, 2, kPlugMemberStride);
        for (std::size_t i = 0; i < 2; ++i) {
            std::size_t row = m + i * kPlugMemberStride;
            t.put<std::uint32_t>(row + kPlugMemberIndexOffset, 500 + 10 * s + i);
            if (i == 0) t.expression(row + kPlugMemberConditionOffset, {{kUnlockReadFlagOpcode, 30 + s}});
        }
    }
    return t;
}

void write(const std::string& dir, const char* name, std::uint8_t op, const std::vector<std::byte>& body,
           const std::vector<std::byte>& prefix = {}) {
    std::FILE* f = std::fopen((dir + "/" + name).c_str(), "wb");
    std::fputc(op, f);
    std::fwrite(prefix.data(), 1, prefix.size(), f);
    std::fwrite(body.data(), 1, body.size(), f);
    std::fclose(f);
}

} // namespace sunrise::middleware::content::packages::tables::items::seedgen

namespace items = sunrise::middleware::content::packages::tables::items;
using namespace items::seedgen;

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const std::string dir = argv[1];
    Blob w = weapon(), p = pursuit(), t = plug_set_table();
    write(dir, "op0", 0, w.b);
    write(dir, "op0-pursuit", 0, p.b);
    write(dir, "op1", 1, w.b);
    write(dir, "op2", 2, w.b);
    // op3: lane, u16 LE definition length, definition, plug-set table.
    std::vector<std::byte> def = w.b, both = w.b;
    both.insert(both.end(), t.b.begin(), t.b.end());
    write(dir, "op3", 3, both,
          {std::byte{0}, std::byte(def.size() & 0xFF), std::byte(def.size() >> 8)});
    write(dir, "op4", 4, t.b);
    // Self-check: the seeds must parse the way they were written.
    items::Row row{};
    std::size_t visited = 0, at = 0;
    const bool ok = items::read_definition(w.b, row) && row.socketCount == 3 && row.statCount == 2
                    && row.equipmentSlot == 2 && row.hasSocketEntryList && row.sandboxPerkCount == 2
                    && row.linkedPlugIndex == 99 && row.insertionFlagSlot == 42 && row.renderOverrideCount == 1
                    && items::find_insertion_rules(w.b, at)
                    && items::visit_allowed_plugs(w.b, t.b, 0, [](void* c, std::uint32_t) noexcept {
                           ++*static_cast<std::size_t*>(c); return true; }, &visited)
                    && visited == 6;
    if (!ok)
        std::fprintf(stderr, "weapon: eq=%d sel=%d linked=%u flag=%d overrides=%u visited=%zu\n",
                     row.equipmentSlot.value_or(-1), row.hasSocketEntryList, row.linkedPlugIndex,
                     row.insertionFlagSlot, row.renderOverrideCount, visited);
    items::Row prow{};
    const bool pok = items::read_definition(p.b, prow) && prow.objectiveMetadataValid && prow.objectiveCount == 2
                     && prow.rewardPreviewsValid && prow.rewardPreviewCount == 1 && prow.lifetimeSeconds == 86400
                     && prow.presenceFlagCount == 2 && prow.itemValueSlotCount == 2 && prow.hasCompletionMetadata;
    std::size_t flags = 0;
    const bool tok = items::visit_plug_set_flags(t.b, [](void* c, std::uint32_t, std::int16_t) noexcept {
        ++*static_cast<std::size_t*>(c); }, &flags) && flags == 4;
    std::printf("weapon=%d pursuit=%d table=%d (sockets=%u stats=%u perks=%u visited=%zu objectives=%u)\n", ok,
                pok, tok, row.socketCount, row.statCount, row.sandboxPerkCount, visited, prow.objectiveCount);
    return ok && pok && tok ? 0 : 1;
}
