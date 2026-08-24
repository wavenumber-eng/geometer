#include <BinMDF_ADriver.hxx>
#include <BinMDF_ADriverTable.hxx>
#include <BinObjMgt_Persistent.hxx>
#include <BinXCAFDrivers.hxx>
#include <BinXCAFDrivers_DocumentRetrievalDriver.hxx>
#include <BinXCAFDrivers_DocumentStorageDriver.hxx>
#include <FSD_BinaryFile.hxx>
#include <Message_Messenger.hxx>
#include <NCollection_IndexedMap.hxx>
#include <PCDM_ReaderStatus.hxx>
#include <PCDM_StoreStatus.hxx>
#include <Standard_GUID.hxx>
#include <Standard_Version.hxx>
#include <Storage_HeaderData.hxx>
#include <Storage_OpenMode.hxx>
#include <TCollection_AsciiString.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDF_Attribute.hxx>
#include <TDF_Label.hxx>
#include <TDF_RelocationTable.hxx>
#include <TDocStd_Document.hxx>
#include <TDocStd_FormatVersion.hxx>
#include <XCAFApp_Application.hxx>
#include <XmlMDF_ADriver.hxx>
#include <XmlMDF_ADriverTable.hxx>
#include <XmlObjMgt.hxx>
#include <XmlObjMgt_Persistent.hxx>
#include <XmlXCAFDrivers.hxx>
#include <XmlXCAFDrivers_DocumentRetrievalDriver.hxx>
#include <XmlXCAFDrivers_DocumentStorageDriver.hxx>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

#if OCC_VERSION_HEX < 0x080000
namespace occ
{
template <class T> using handle = opencascade::handle<T>;

template <class Target, class Source> handle<Target> down_cast(const handle<Source>& source)
{
    return handle<Target>::DownCast(source);
}
} // namespace occ
#endif

namespace
{

constexpr const char* kProbeValue = "geometer-custom-driver-payload-a0";

struct TemporaryDirectory
{
    std::filesystem::path path;

    explicit TemporaryDirectory(const std::string& prefix)
    {
        const auto nonce = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        for (unsigned int attempt = 0; attempt < 256; ++attempt)
        {
            path = std::filesystem::temp_directory_path() /
                   (prefix + std::to_string(nonce) + "-" + std::to_string(attempt));
            std::error_code error;
            if (std::filesystem::create_directory(path, error))
                return;
            if (error)
                throw std::filesystem::filesystem_error(
                    "failed creating isolated temporary directory", path, error);
        }
        throw std::runtime_error("failed finding an unused isolated temporary directory");
    }

    TemporaryDirectory(const TemporaryDirectory&) = delete;
    TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;

    ~TemporaryDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
};

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

class GeometerResearchProbe final : public TDF_Attribute
{
  public:
    static const Standard_GUID& GetID()
    {
        // Wavenumber-owned research-only GUID. It is not a production annotation contract.
        static const Standard_GUID id("eacfa1c8-42b2-4b9e-9d69-e3f050eaf8a1");
        return id;
    }

    static occ::handle<GeometerResearchProbe> Set(const TDF_Label& label,
                                                  const TCollection_AsciiString& value)
    {
        occ::handle<GeometerResearchProbe> attribute;
        if (!label.FindAttribute(GetID(), attribute))
        {
            attribute = new GeometerResearchProbe();
            label.AddAttribute(attribute);
        }
        attribute->SetValue(value);
        return attribute;
    }

    const Standard_GUID& ID() const override
    {
        return GetID();
    }

    const TCollection_AsciiString& Value() const
    {
        return value_;
    }

    void SetValue(const TCollection_AsciiString& value)
    {
        if (value_ == value)
            return;
        Backup();
        value_ = value;
    }

    occ::handle<TDF_Attribute> NewEmpty() const override
    {
        return new GeometerResearchProbe();
    }

    void Restore(const occ::handle<TDF_Attribute>& other) override
    {
        value_ = occ::down_cast<GeometerResearchProbe>(other)->Value();
    }

    void Paste(const occ::handle<TDF_Attribute>& target,
               const occ::handle<TDF_RelocationTable>&) const override
    {
        occ::down_cast<GeometerResearchProbe>(target)->SetValue(value_);
    }

    DEFINE_STANDARD_RTTIEXT(GeometerResearchProbe, TDF_Attribute)

  private:
    TCollection_AsciiString value_;
};

IMPLEMENT_STANDARD_RTTIEXT(GeometerResearchProbe, TDF_Attribute)

class GeometerBinResearchProbeDriver final : public BinMDF_ADriver
{
  public:
    explicit GeometerBinResearchProbeDriver(const occ::handle<Message_Messenger>& message_driver)
        : BinMDF_ADriver(message_driver, STANDARD_TYPE(GeometerResearchProbe)->Name())
    {
    }

    occ::handle<TDF_Attribute> NewEmpty() const override
    {
        return new GeometerResearchProbe();
    }

    bool Paste(const BinObjMgt_Persistent& source, const occ::handle<TDF_Attribute>& target,
               BinObjMgt_RRelocationTable&) const override
    {
        TCollection_AsciiString value;
        if (!(source >> value))
            return false;
        occ::down_cast<GeometerResearchProbe>(target)->SetValue(value);
        return true;
    }

    void Paste(const occ::handle<TDF_Attribute>& source, BinObjMgt_Persistent& target,
               NCollection_IndexedMap<occ::handle<Standard_Transient>>&) const override
    {
        target << occ::down_cast<GeometerResearchProbe>(source)->Value();
    }
};

class GeometerXmlResearchProbeDriver final : public XmlMDF_ADriver
{
  public:
    explicit GeometerXmlResearchProbeDriver(const occ::handle<Message_Messenger>& message_driver)
        : XmlMDF_ADriver(message_driver, "wn", "research-probe")
    {
    }

    occ::handle<TDF_Attribute> NewEmpty() const override
    {
        return new GeometerResearchProbe();
    }

    bool Paste(const XmlObjMgt_Persistent& source, const occ::handle<TDF_Attribute>& target,
               XmlObjMgt_RRelocationTable&) const override
    {
        occ::down_cast<GeometerResearchProbe>(target)->SetValue(
            TCollection_AsciiString(XmlObjMgt::GetStringValue(source).GetString()));
        return true;
    }

    void Paste(const occ::handle<TDF_Attribute>& source, XmlObjMgt_Persistent& target,
               XmlObjMgt_SRelocationTable&) const override
    {
        XmlObjMgt::SetStringValue(
            target, occ::down_cast<GeometerResearchProbe>(source)->Value().ToCString());
    }
};

class GeometerBinStorageDriver final : public BinXCAFDrivers_DocumentStorageDriver
{
  public:
    occ::handle<BinMDF_ADriverTable>
    AttributeDrivers(const occ::handle<Message_Messenger>& message_driver) override
    {
        occ::handle<BinMDF_ADriverTable> table = BinXCAFDrivers::AttributeDrivers(message_driver);
        table->AddDriver(new GeometerBinResearchProbeDriver(message_driver));
        return table;
    }
};

class GeometerBinRetrievalDriver final : public BinXCAFDrivers_DocumentRetrievalDriver
{
  public:
    occ::handle<BinMDF_ADriverTable>
    AttributeDrivers(const occ::handle<Message_Messenger>& message_driver) override
    {
        occ::handle<BinMDF_ADriverTable> table = BinXCAFDrivers::AttributeDrivers(message_driver);
        table->AddDriver(new GeometerBinResearchProbeDriver(message_driver));
        return table;
    }
};

class GeometerXmlStorageDriver final : public XmlXCAFDrivers_DocumentStorageDriver
{
  public:
    GeometerXmlStorageDriver()
        : XmlXCAFDrivers_DocumentStorageDriver("Wavenumber Geometer research probe")
    {
        AddNamespace("wn", "https://wavenumber.com/ns/geometer/research/ocaf/a0");
    }

    occ::handle<XmlMDF_ADriverTable>
    AttributeDrivers(const occ::handle<Message_Messenger>& message_driver) override
    {
        occ::handle<XmlMDF_ADriverTable> table =
            XmlXCAFDrivers_DocumentStorageDriver::AttributeDrivers(message_driver);
        table->AddDriver(new GeometerXmlResearchProbeDriver(message_driver));
        return table;
    }
};

class GeometerXmlRetrievalDriver final : public XmlXCAFDrivers_DocumentRetrievalDriver
{
  public:
    occ::handle<XmlMDF_ADriverTable>
    AttributeDrivers(const occ::handle<Message_Messenger>& message_driver) override
    {
        occ::handle<XmlMDF_ADriverTable> table =
            XmlXCAFDrivers_DocumentRetrievalDriver::AttributeDrivers(message_driver);
        table->AddDriver(new GeometerXmlResearchProbeDriver(message_driver));
        return table;
    }
};

void define_probe_format(const occ::handle<XCAFApp_Application>& application,
                         const std::string& format, const std::string& extension, bool binary,
                         bool include_retrieval_driver)
{
    occ::handle<PCDM_RetrievalDriver> reader;
    occ::handle<PCDM_StorageDriver> writer;
    if (binary)
    {
        reader =
            include_retrieval_driver
                ? occ::handle<PCDM_RetrievalDriver>(new GeometerBinRetrievalDriver())
                : occ::handle<PCDM_RetrievalDriver>(new BinXCAFDrivers_DocumentRetrievalDriver());
        writer = new GeometerBinStorageDriver();
    }
    else
    {
        reader =
            include_retrieval_driver
                ? occ::handle<PCDM_RetrievalDriver>(new GeometerXmlRetrievalDriver())
                : occ::handle<PCDM_RetrievalDriver>(new XmlXCAFDrivers_DocumentRetrievalDriver());
        writer = new GeometerXmlStorageDriver();
    }
    application->DefineFormat(format.c_str(), "Geometer custom OCAF research probe",
                              extension.c_str(), reader, writer);
}

int serialized_storage_version(const std::filesystem::path& artifact, bool binary)
{
    if (binary)
    {
        const occ::handle<FSD_BinaryFile> driver = new FSD_BinaryFile();
        require(driver->Open(TCollection_AsciiString(artifact.u8string().c_str()),
                             Storage_VSRead) == Storage_VSOk,
                "failed opening custom binary XCAF header");
        const occ::handle<Storage_HeaderData> header = new Storage_HeaderData();
        const bool read = header->Read(driver);
        driver->Close();
        require(read && header->StorageVersion().IsIntegerValue(),
                "custom binary XCAF header has no integer storage version");
        return header->StorageVersion().IntegerValue();
    }

    std::ifstream input(artifact, std::ios::binary);
    require(input.good(), "failed opening custom XML XCAF artifact");
    const std::string text((std::istreambuf_iterator<char>(input)),
                           std::istreambuf_iterator<char>());
    constexpr const char* marker = "DocVersion=\"";
    const std::size_t start = text.find(marker);
    require(start != std::string::npos, "custom XML XCAF header has no DocVersion attribute");
    const std::size_t value_start = start + std::char_traits<char>::length(marker);
    const std::size_t value_end = text.find('"', value_start);
    require(value_end != std::string::npos && value_end > value_start,
            "custom XML XCAF DocVersion attribute is malformed");
    return std::stoi(text.substr(value_start, value_end - value_start));
}

std::filesystem::path write_probe(const occ::handle<XCAFApp_Application>& application,
                                  const std::filesystem::path& output_directory,
                                  const std::string& format, const std::string& extension)
{
    occ::handle<TDocStd_Document> document;
    application->NewDocument(TCollection_ExtendedString(format.c_str(), true), document);
    require(!document.IsNull(), format + " document creation failed");
    GeometerResearchProbe::Set(document->Main(), kProbeValue);
    const std::filesystem::path artifact = output_directory / (format + "." + extension);
    require(application->SaveAs(document, TCollection_ExtendedString(artifact.u8string().c_str(),
                                                                     true)) == PCDM_SS_OK,
            format + " custom attribute storage failed");
    require(serialized_storage_version(artifact, extension.find("xbf") != std::string::npos) ==
                TDocStd_FormatVersion_CURRENT,
            format + " serialized an unexpected OCAF storage version");
    application->Close(document);
    return artifact;
}

void require_probe_reload(const occ::handle<XCAFApp_Application>& application,
                          const std::filesystem::path& artifact, const std::string& format)
{
    occ::handle<TDocStd_Document> restored;
    require(application->Open(TCollection_ExtendedString(artifact.u8string().c_str(), true),
                              restored) == PCDM_RS_OK &&
                !restored.IsNull(),
            format + " custom attribute retrieval failed");
    occ::handle<GeometerResearchProbe> probe;
    require(restored->Main().FindAttribute(GeometerResearchProbe::GetID(), probe) &&
                probe->Value().IsEqual(kProbeValue),
            format + " changed or lost the custom attribute payload");
    application->Close(restored);
}

void require_missing_retrieval_driver_loses_probe(
    const occ::handle<XCAFApp_Application>& application, const TemporaryDirectory& temporary,
    const std::string& format, const std::string& extension)
{
    const std::filesystem::path artifact =
        write_probe(application, temporary.path, format, extension);
    occ::handle<TDocStd_Document> restored;
    const PCDM_ReaderStatus status =
        application->Open(TCollection_ExtendedString(artifact.u8string().c_str(), true), restored);
    require(status == PCDM_RS_OK && !restored.IsNull(),
            format + " missing-driver document did not exhibit the expected readable-loss mode");
    occ::handle<GeometerResearchProbe> probe;
    require(!restored->Main().FindAttribute(GeometerResearchProbe::GetID(), probe),
            format + " unexpectedly restored a custom attribute without its retrieval driver");
    application->Close(restored);
}

} // namespace

int main(int argc, char** argv)
{
    try
    {
        static_assert(TDocStd_FormatVersion_CURRENT == TDocStd_FormatVersion_VERSION_12);
        const occ::handle<XCAFApp_Application> application = XCAFApp_Application::GetApplication();
        if (argc == 2 && std::string(argv[1]) == "--version")
        {
            std::cout << OCC_VERSION_COMPLETE << '\n';
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--write-matrix")
        {
            const std::filesystem::path output_directory = argv[2];
            require(std::filesystem::create_directories(output_directory) ||
                        std::filesystem::is_directory(output_directory),
                    "failed creating matrix output directory");
            define_probe_format(application, "GeometerBinXCAFProbe", "wnxbf", true, true);
            define_probe_format(application, "GeometerXmlXCAFProbe", "wnxml", false, true);
            write_probe(application, output_directory, "GeometerBinXCAFProbe", "wnxbf");
            write_probe(application, output_directory, "GeometerXmlXCAFProbe", "wnxml");
            std::cout << "wrote OCCT " << OCC_VERSION_COMPLETE
                      << " custom-driver matrix artifacts\n";
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--read-matrix")
        {
            const std::filesystem::path input_directory = argv[2];
            define_probe_format(application, "GeometerBinXCAFProbe", "wnxbf", true, true);
            define_probe_format(application, "GeometerXmlXCAFProbe", "wnxml", false, true);
            require_probe_reload(application, input_directory / "GeometerBinXCAFProbe.wnxbf",
                                 "GeometerBinXCAFProbe");
            require_probe_reload(application, input_directory / "GeometerXmlXCAFProbe.wnxml",
                                 "GeometerXmlXCAFProbe");
            std::cout << "read OCCT " << OCC_VERSION_COMPLETE
                      << " custom-driver matrix artifacts\n";
            return 0;
        }
        require(argc == 1, "usage: geometer_step_topology_xcaf_custom_driver_test "
                           "[--version | --write-matrix|--read-matrix <directory>]");
        define_probe_format(application, "GeometerBinXCAFProbe", "wnxbf", true, true);
        define_probe_format(application, "GeometerXmlXCAFProbe", "wnxml", false, true);
        define_probe_format(application, "GeometerBinXCAFMissingProbe", "wnmissingxbf", true,
                            false);
        define_probe_format(application, "GeometerXmlXCAFMissingProbe", "wnmissingxml", false,
                            false);

        TemporaryDirectory temporary("geometer-xcaf-custom-driver-");

        const std::filesystem::path binary =
            write_probe(application, temporary.path, "GeometerBinXCAFProbe", "wnxbf");
        require_probe_reload(application, binary, "GeometerBinXCAFProbe");
        const std::filesystem::path xml =
            write_probe(application, temporary.path, "GeometerXmlXCAFProbe", "wnxml");
        require_probe_reload(application, xml, "GeometerXmlXCAFProbe");
        require_missing_retrieval_driver_loses_probe(application, temporary,
                                                     "GeometerBinXCAFMissingProbe", "wnmissingxbf");
        require_missing_retrieval_driver_loses_probe(application, temporary,
                                                     "GeometerXmlXCAFMissingProbe", "wnmissingxml");

        std::cout << "XCAF custom binary/XML attribute driver probe passed\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
