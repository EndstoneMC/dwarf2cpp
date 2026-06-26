#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/DebugInfo/DWARF/DWARFContext.h>
#include <llvm/DebugInfo/DWARF/DWARFDie.h>
#include <llvm/DebugInfo/DWARF/DWARFTypePrinter.h>
#include <llvm/DebugInfo/DWARF/DWARFTypeUnit.h>
#include <llvm/Demangle/Demangle.h>

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

namespace {
std::string ToString(llvm::dwarf::Attribute attr) {
    static const std::unordered_map<llvm::dwarf::Attribute, std::string> map = {
#define HANDLE_DW_AT(ID, NAME, VERSION, VENDOR) {llvm::dwarf::DW_AT_##NAME, "DW_AT_" #NAME},
#include "llvm/BinaryFormat/Dwarf.def"

#undef HANDLE_DW_AT
    };
    return map.at(attr);
}

llvm::dwarf::Attribute ToAttribute(const std::string &key) {
    static const std::unordered_map<std::string, llvm::dwarf::Attribute> map = {
#define HANDLE_DW_AT(ID, NAME, VERSION, VENDOR) {"DW_AT_" #NAME, llvm::dwarf::DW_AT_##NAME},
#include "llvm/BinaryFormat/Dwarf.def"

#undef HANDLE_DW_AT
    };
    return map.at(key);
}

// POLYFILL BEGINS TODO: remove after updating to LLVM 20
llvm::DWARFDie getAttributeValueAsReferencedDie(const llvm::DWARFDie &die,
                                                const llvm::DWARFFormValue &V) {
    llvm::DWARFDie Result;
    if (std::optional<uint64_t> Offset = V.getAsRelativeReference()) {
        Result = const_cast<llvm::DWARFUnit *>(V.getUnit())
                     ->getDIEForOffset(V.getUnit()->getOffset() + *Offset);
    } else if (Offset = V.getAsDebugInfoReference(); Offset) {
        if (llvm::DWARFUnit *SpecUnit
            = die.getDwarfUnit()->getUnitVector().getUnitForOffset(*Offset))
            Result = SpecUnit->getDIEForOffset(*Offset);
    } else if (std::optional<uint64_t> Sig = V.getAsSignatureReference()) {
        if (llvm::DWARFTypeUnit *TU = die.getDwarfUnit()->getContext().getTypeUnitForHash(
                die.getDwarfUnit()->getVersion(), *Sig, die.getDwarfUnit()->isDWOUnit()))
            Result = TU->getDIEForOffset(TU->getTypeOffset() + TU->getOffset());
    }
    return Result;
}

llvm::DWARFDie getAttributeValueAsReferencedDie(const llvm::DWARFDie &die,
                                                llvm::dwarf::Attribute Attr) {
    if (std::optional<llvm::DWARFFormValue> F = die.find(Attr))
        return getAttributeValueAsReferencedDie(die, *F);
    return {};
}

std::optional<llvm::DWARFFormValue> findRecursively(const llvm::DWARFDie &die,
                                                    llvm::ArrayRef<llvm::dwarf::Attribute> Attrs) {
    // polyfill from LLVM 20
    llvm::SmallVector<llvm::DWARFDie, 3> Worklist;
    Worklist.push_back(die);
    llvm::SmallSet<llvm::DWARFDie, 3> Seen;
    Seen.insert(die);

    while (!Worklist.empty()) {
        llvm::DWARFDie Die = Worklist.pop_back_val();
        if (!Die.isValid()) {
            continue;
        }
        if (auto Value = Die.find(Attrs)) {
            return Value;
        }
        for (llvm::dwarf::Attribute Attr : {llvm::dwarf::DW_AT_abstract_origin,
                                            llvm::dwarf::DW_AT_specification,
                                            llvm::dwarf::DW_AT_signature}) {
            if (auto D = getAttributeValueAsReferencedDie(Die, Attr)) {
                if (Seen.insert(D).second) {
                    Worklist.push_back(D);
                }
            }
        }
    }

    return std::nullopt;
};
// POLYFILL ENDS TODO: remove after updating to LLVM 20
} // namespace

class PyDWARFContext {
public:
    explicit PyDWARFContext(const std::string &path) {
        auto result = llvm::object::ObjectFile::createObjectFile(path);
        if (!result) {
            throw std::runtime_error(toString(result.takeError()));
        }
        object_ = std::move(*result);
        context_ = llvm::DWARFContext::create(*object_.getBinary());
    }

    [[nodiscard]] auto info_section_units() const {
        std::vector<llvm::DWARFUnit *> units;
        for (const auto &unit : context_->info_section_units()) {
            units.push_back(unit.get());
        }
        return units;
    }

    [[nodiscard]] auto types_section_units() const {
        std::vector<llvm::DWARFUnit *> units;
        for (const auto &unit : context_->types_section_units()) {
            units.push_back(unit.get());
        }
        return units;
    }

    [[nodiscard]] auto compile_units() const {
        std::vector<llvm::DWARFUnit *> units;
        for (const auto &unit : context_->compile_units()) {
            units.push_back(unit.get());
        }
        return units;
    }

    [[nodiscard]] auto getNumCompileUnits() const { return context_->getNumCompileUnits(); }

    [[nodiscard]] auto getNumTypeUnits() const { return context_->getNumTypeUnits(); }

    [[nodiscard]] auto getNumDWOCompileUnits() const { return context_->getNumDWOCompileUnits(); }

    [[nodiscard]] auto getNumDWOTypeUnits() const { return context_->getNumDWOTypeUnits(); }

    [[nodiscard]] auto getMaxVersion() const { return context_->getMaxVersion(); }

    [[nodiscard]] auto getMaxDWOVersion() const { return context_->getMaxDWOVersion(); }

    [[nodiscard]] auto isLittleEndian() const { return context_->isLittleEndian(); }

    [[nodiscard]] auto getCUAddrSize() const { return context_->getCUAddrSize(); }

private:
    llvm::object::OwningBinary<llvm::object::ObjectFile> object_;
    std::unique_ptr<llvm::DWARFContext> context_;
};

class PyDWARFTypePrinter {
public:
    PyDWARFTypePrinter() : os(buffer), printer(os) {}
    std::string string() {
        os.flush();
        return buffer;
    }
    auto appendQualifiedName(llvm::DWARFDie die) { printer.appendQualifiedName(die); }
    llvm::DWARFDie appendQualifiedNameBefore(llvm::DWARFDie die) {
        return printer.appendQualifiedNameBefore(die);
    }
    auto appendUnqualifiedName(llvm::DWARFDie die) { printer.appendUnqualifiedName(die); }
    auto appendUnqualifiedNameBefore(llvm::DWARFDie die) {
        return printer.appendUnqualifiedNameBefore(die);
    }
    auto appendUnqualifiedNameAfter(llvm::DWARFDie die, llvm::DWARFDie inner) {
        printer.appendUnqualifiedNameAfter(die, inner);
    }
    auto appendScopes(llvm::DWARFDie die) { printer.appendScopes(die); }

private:
    std::string buffer;
    llvm::raw_string_ostream os;
    llvm::DWARFTypePrinter<llvm::DWARFDie> printer;
};

NB_MODULE(_dwarf, m) {
    nb::enum_<llvm::dwarf::AccessAttribute>(m, "AccessAttribute", nb::is_arithmetic())
        .value("PUBLIC", llvm::dwarf::DW_ACCESS_public)
        .value("PROTECTED", llvm::dwarf::DW_ACCESS_protected)
        .value("PRIVATE", llvm::dwarf::DW_ACCESS_private);

    nb::enum_<llvm::dwarf::VirtualityAttribute>(m, "VirtualityAttribute", nb::is_arithmetic())
        .value("NONE", llvm::dwarf::DW_VIRTUALITY_none)
        .value("VIRTUAL", llvm::dwarf::DW_VIRTUALITY_virtual)
        .value("PURE_VIRTUAL", llvm::dwarf::DW_VIRTUALITY_pure_virtual);

    nb::enum_<llvm::dwarf::InlineAttribute>(m, "InlineAttribute", nb::is_arithmetic())
        .value("NOT_INLINED", llvm::dwarf::DW_INL_not_inlined)
        .value("INLINED", llvm::dwarf::DW_INL_inlined)
        .value("DECLARED_NOT_INLINED", llvm::dwarf::DW_INL_declared_not_inlined)
        .value("DECLARED_INLINED", llvm::dwarf::DW_INL_declared_inlined);

    nb::class_<PyDWARFContext>(m, "DWARFContext")
        .def(nb::init<const std::string &>(), nb::arg("path"))
        .def_prop_ro("info_section_units",
                     &PyDWARFContext::info_section_units,
                     nb::rv_policy::reference_internal)
        .def_prop_ro("types_section_units",
                     &PyDWARFContext::types_section_units,
                     nb::rv_policy::reference_internal)
        .def_prop_ro("compile_units",
                     &PyDWARFContext::compile_units,
                     nb::rv_policy::reference_internal)
        .def_prop_ro("num_compile_units", &PyDWARFContext::getNumCompileUnits)
        .def_prop_ro("num_type_units", &PyDWARFContext::getNumTypeUnits)
        .def_prop_ro("num_dwo_compile_units", &PyDWARFContext::getNumDWOCompileUnits)
        .def_prop_ro("num_dwo_type_units", &PyDWARFContext::getNumDWOTypeUnits)
        .def_prop_ro("max_version", &PyDWARFContext::getMaxVersion)
        .def_prop_ro("max_dwo_version", &PyDWARFContext::getMaxDWOVersion)
        .def_prop_ro("is_little_endian", &PyDWARFContext::isLittleEndian)
        .def_prop_ro("cu_addr_size", &PyDWARFContext::getCUAddrSize);

    nb::class_<llvm::DWARFUnit>(m, "DWARFUnit")
        .def_prop_ro("offset", &llvm::DWARFUnit::getOffset)
        .def_prop_ro("length", &llvm::DWARFUnit::getLength)
        .def_prop_ro("is_type_unit", &llvm::DWARFUnit::isTypeUnit)
        .def_prop_ro("unit_die",
                     [](llvm::DWARFUnit &self) -> std::optional<llvm::DWARFDie> {
                         if (auto die = self.getUnitDIE(false); die.isValid()) {
                             return die;
                         }
                         return std::nullopt;
                     })
        .def_prop_ro("compilation_dir", &llvm::DWARFUnit::getCompilationDir);

    nb::class_<llvm::DWARFDie>(m, "DWARFDie")
        .def_prop_ro("unit", &llvm::DWARFDie::getDwarfUnit)
        .def_prop_ro("offset", &llvm::DWARFDie::getOffset)
        .def_prop_ro(
            "tag", [](const llvm::DWARFDie &self) { return TagString(self.getTag()).str(); })
        .def_prop_ro("parent",
                     [](const llvm::DWARFDie &self) -> std::optional<llvm::DWARFDie> {
                         if (auto parent = self.getParent(); parent.isValid()) {
                             return parent;
                         }
                         return std::nullopt;
                     })
        .def_prop_ro("short_name",
                     [](const llvm::DWARFDie &self) {
                         return llvm::dwarf::toString(
                             findRecursively(self, llvm::dwarf::DW_AT_name), nullptr);
                     })
        .def_prop_ro("linkage_name",
                     [](const llvm::DWARFDie &self) {
                         return llvm::dwarf::toString(
                             findRecursively(self,
                                             {llvm::dwarf::DW_AT_MIPS_linkage_name,
                                              llvm::dwarf::DW_AT_linkage_name}),
                             nullptr);
                     })
        .def_prop_ro("decl_line",
                     [](const llvm::DWARFDie &self) {
                         return llvm::dwarf::toUnsigned(
                             findRecursively(self, llvm::dwarf::DW_AT_decl_line), 0);
                     })
        .def_prop_ro(
            "decl_file",
            [](const llvm::DWARFDie &self) -> std::optional<std::string> {
                if (auto form = findRecursively(self, llvm::dwarf::DW_AT_decl_file))
                    return form->getAsFile(
                        llvm::DILineInfoSpecifier::FileLineInfoKind::AbsoluteFilePath);
                return std::nullopt;
            })
        .def_prop_ro("attributes",
                     [](const llvm::DWARFDie &self) {
                         std::vector<llvm::DWARFAttribute> attrs;
                         for (const auto &attr : self.attributes()) {
                             attrs.emplace_back(attr);
                         }
                         return attrs;
                     })
        .def_prop_ro("children",
                     [](const llvm::DWARFDie &self) {
                         std::vector<llvm::DWARFDie> children;
                         for (const auto &child : self.children()) {
                             if (child.isValid()) {
                                 children.emplace_back(child);
                             }
                         }
                         return children;
                     })
        .def("dump",
             [](const llvm::DWARFDie &self) {
                 std::string result;
                 llvm::raw_string_ostream os(result);
                 self.dump(os);
                 os.flush();
                 return result;
             })
        .def("find",
             [](const llvm::DWARFDie &self, const std::string &attribute) {
                 return self.find(ToAttribute(attribute));
             })
        .def("resolve_type_unit_reference",
             [](const llvm::DWARFDie &self) -> std::optional<llvm::DWARFDie> {
                 auto die = self.resolveTypeUnitReference();
                 if (!die.isValid()) {
                     return std::nullopt;
                 }
                 return die;
             })
        .def("__hash__", &llvm::DWARFDie::getOffset)
        .def(nb::self == nb::self);

    nb::class_<llvm::DWARFAttribute>(m, "DWARFAttribute")
        .def_ro("offset", &llvm::DWARFAttribute::Offset)
        .def_ro("byte_size", &llvm::DWARFAttribute::ByteSize)
        .def_prop_ro("name",
                     [](const llvm::DWARFAttribute &self) { return ToString(self.Attr); })
        .def_ro("value", &llvm::DWARFAttribute::Value);

    nb::class_<llvm::DWARFFormValue>(m, "DWARFFormValue")
        .def_prop_ro("form",
                     [](const llvm::DWARFFormValue &self) {
                         return FormEncodingString(self.getForm()).str();
                     })
        .def("as_referenced_die",
             [](const llvm::DWARFFormValue &self) -> std::optional<llvm::DWARFDie> {
                 llvm::DWARFDie result;
                 auto &V = self;
                 auto *U = self.getUnit();
                 if (std::optional<uint64_t> offset = V.getAsRelativeReference()) {
                     result = const_cast<llvm::DWARFUnit *>(V.getUnit())
                                  ->getDIEForOffset(V.getUnit()->getOffset() + offset.value());
                 } else if (offset = V.getAsDebugInfoReference(); offset) {
                     if (llvm::DWARFUnit *spec_unit
                         = U->getUnitVector().getUnitForOffset(offset.value()))
                         result = spec_unit->getDIEForOffset(*offset);
                 } else if (std::optional<uint64_t> sig = V.getAsSignatureReference()) {
                     if (llvm::DWARFTypeUnit *TU = U->getContext().getTypeUnitForHash(
                             U->getVersion(), sig.value(), U->isDWOUnit()))
                         result = TU->getDIEForOffset(TU->getTypeOffset() + TU->getOffset());
                 }

                 if (result.isValid()) {
                     return result;
                 }
                 return std::nullopt;
             })
        .def("as_string",
             [](const llvm::DWARFFormValue &self) -> std::string {
                 auto e = self.getAsCString();
                 if (e) {
                     return e.get();
                 }
                 std::string message = toString(e.takeError());
                 throw nb::value_error(message.c_str());
             })
        .def("as_constant", [](const llvm::DWARFFormValue &self) -> nb::int_ {
            if (auto s = self.getAsSignedConstant(); s) {
                return nb::int_(s.value());
            }
            if (auto u = self.getAsUnsignedConstant(); u) {
                return nb::int_(u.value());
            }
            throw nb::value_error("Invalid constant value");
        });

    nb::class_<PyDWARFTypePrinter>(m, "DWARFTypePrinter")
        .def(nb::init())
        .def("append_qualified_name", &PyDWARFTypePrinter::appendQualifiedName)
        .def("append_qualified_name_before", &PyDWARFTypePrinter::appendQualifiedNameBefore)
        .def("append_unqualified_name", &PyDWARFTypePrinter::appendUnqualifiedName)
        .def("append_unqualified_name_before", &PyDWARFTypePrinter::appendUnqualifiedNameBefore)
        .def("append_unqualified_name_after", &PyDWARFTypePrinter::appendUnqualifiedNameAfter)
        .def("append_scopes", &PyDWARFTypePrinter::appendScopes)
        .def("__str__", &PyDWARFTypePrinter::string);
}
