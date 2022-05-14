#include <Catch2/Catch2.hpp>


#include "Poppy/IconConfig.hpp"

namespace { extern char const* const IconConfig; }


TEST_CASE("IconConfig") {
	
	poppy::IconConfig::init(::IconConfig);
	CHECK(poppy::IconConfig::unicode("down-big") == 59392);
	CHECK(poppy::IconConfig::unicode("align-right") == 59398);
	
}


namespace {

	char const* const IconConfig = R"(
	{
	  "name": "",
	  "css_prefix_text": "icon-",
	  "css_use_suffix": false,
	  "hinting": true,
	  "units_per_em": 1000,
	  "ascent": 850,
	  "glyphs": [
		{
		  "uid": "1c4068ed75209e21af36017df8871802",
		  "css": "down-big",
		  "code": 59392,
		  "src": "fontawesome"
		},
		{
		  "uid": "555ef8c86832e686fef85f7af2eb7cde",
		  "css": "left-big",
		  "code": 59393,
		  "src": "fontawesome"
		},
		{
		  "uid": "ad6b3fbb5324abe71a9c0b6609cbb9f1",
		  "css": "right-big",
		  "code": 59394,
		  "src": "fontawesome"
		},
		{
		  "uid": "95376bf082bfec6ce06ea1cda7bd7ead",
		  "css": "up-big",
		  "code": 59395,
		  "src": "fontawesome"
		},
		{
		  "uid": "c62cbc7fa604e32cc19731a3bb29be79",
		  "css": "map-o",
		  "code": 62072,
		  "src": "fontawesome"
		},
		{
		  "uid": "79beb8c7fcd56e1985b70e9a120cbcbf",
		  "css": "map",
		  "code": 62073,
		  "src": "fontawesome"
		},
		{
		  "uid": "af95ef0ddda80a78828c62d386506433",
		  "css": "cubes",
		  "code": 61875,
		  "src": "fontawesome"
		},
		{
		  "uid": "fbede3c5584282a0e9b131926dff71da",
		  "css": "cube",
		  "code": 61874,
		  "src": "fontawesome"
		},
		{
		  "uid": "0cd2582b8c93719d066ee0affd02ac78",
		  "css": "sort-alt-up",
		  "code": 61792,
		  "src": "fontawesome"
		},
		{
		  "uid": "27b13eff5eb0ca15e01a6e65ffe6eeec",
		  "css": "sort-alt-down",
		  "code": 61793,
		  "src": "fontawesome"
		},
		{
		  "uid": "f4f0e849b805be1f6d76b65581cb3b8b",
		  "css": "align-left",
		  "code": 59396,
		  "src": "fontawesome"
		},
		{
		  "uid": "ae6336c46d73af999fe7460c089abb4d",
		  "css": "align-center",
		  "code": 59397,
		  "src": "fontawesome"
		},
		{
		  "uid": "e1e7306b47c3c5e6faecce9d32571381",
		  "css": "align-right",
		  "code": 59398,
		  "src": "fontawesome"
		},
		{
		  "uid": "25a81737628d1e654a30ad412d7d6dd7",
		  "css": "align-justify",
		  "code": 59399,
		  "src": "fontawesome"
		},
		{
		  "uid": "48b87105bd38c20315f1b705b8c7b38c",
		  "css": "list",
		  "code": 59400,
		  "src": "fontawesome"
		},
		{
		  "uid": "594e9271c08ff732c04b3bf3117b9040",
		  "css": "indent-left",
		  "code": 59401,
		  "src": "fontawesome"
		},
		{
		  "uid": "4d2dfc45d8176b1f26aed973fa84a91e",
		  "css": "indent-right",
		  "code": 59402,
		  "src": "fontawesome"
		},
		{
		  "uid": "a2a74f5e7b7d9ba054897d8c795a326a",
		  "css": "list-bullet",
		  "code": 61642,
		  "src": "fontawesome"
		},
		{
		  "uid": "f6766a8b042c2453a4e153af03294383",
		  "css": "list-numbered",
		  "code": 61643,
		  "src": "fontawesome"
		},
		{
		  "uid": "cfb6e143c0650c485536374f2a2f88fb",
		  "css": "television",
		  "code": 62060,
		  "src": "fontawesome"
		},
		{
		  "uid": "e99461abfef3923546da8d745372c995",
		  "css": "cog",
		  "code": 59403,
		  "src": "fontawesome"
		},
		{
		  "uid": "b1887b423d2fd15c345e090320c91ca0",
		  "css": "th",
		  "code": 59404,
		  "src": "fontawesome"
		},
		{
		  "uid": "dd492243d64e21dfe16a92452f7861cb",
		  "css": "th-large",
		  "code": 59405,
		  "src": "fontawesome"
		},
		{
		  "uid": "f805bb95d40c7ef2bc51b3d50d4f2e5c",
		  "css": "th-list",
		  "code": 59406,
		  "src": "fontawesome"
		},
		{
		  "uid": "12f4ece88e46abd864e40b35e05b11cd",
		  "css": "ok",
		  "code": 59407,
		  "src": "fontawesome"
		},
		{
		  "uid": "b013f6403e5ab0326614e68d1850fd6b",
		  "css": "resize-full-alt",
		  "code": 61618,
		  "src": "fontawesome"
		},
		{
		  "uid": "e594fc6e5870b4ab7e49f52571d52577",
		  "css": "resize-full",
		  "code": 59408,
		  "src": "fontawesome"
		},
		{
		  "uid": "3c24ee33c9487bbf18796ca6dffa1905",
		  "css": "resize-small",
		  "code": 59409,
		  "src": "fontawesome"
		},
		{
		  "uid": "d3b3f17bc3eb7cd809a07bbd4d178bee",
		  "css": "resize-vertical",
		  "code": 59410,
		  "src": "fontawesome"
		},
		{
		  "uid": "3c73d058e4589b65a8d959c0fc8f153d",
		  "css": "resize-horizontal",
		  "code": 59411,
		  "src": "fontawesome"
		},
		{
		  "uid": "6605ee6441bf499ffa3c63d3c7409471",
		  "css": "move",
		  "code": 61511,
		  "src": "fontawesome"
		},
		{
		  "uid": "0b2b66e526028a6972d51a6f10281b4b",
		  "css": "zoom-in",
		  "code": 59412,
		  "src": "fontawesome"
		},
		{
		  "uid": "d25d10efa900f529ad1d275657cfd30e",
		  "css": "zoom-out",
		  "code": 59413,
		  "src": "fontawesome"
		},
		{
		  "uid": "b624a1e512819d410ddbee84e6918b9d",
		  "css": "stop",
		  "code": 59414,
		  "src": "fontawesome"
		},
		{
		  "uid": "ce06b5805120d0c2f8d60cd3f1a4fdb5",
		  "css": "play",
		  "code": 59415,
		  "src": "fontawesome"
		},
		{
		  "uid": "0b28050bac9d3facf2f0226db643ece0",
		  "css": "pause",
		  "code": 59416,
		  "src": "fontawesome"
		},
		{
		  "uid": "559647a6f430b3aeadbecd67194451dd",
		  "css": "menu",
		  "code": 61641,
		  "src": "fontawesome"
		},
		{
		  "uid": "f8aa663c489bcbd6e68ec8147dca841e",
		  "css": "folder",
		  "code": 59417,
		  "src": "fontawesome"
		},
		{
		  "uid": "c95735c17a10af81448c7fed98a04546",
		  "css": "folder-open",
		  "code": 59418,
		  "src": "fontawesome"
		},
		{
		  "uid": "b091a8bd0fdade174951f17d936f51e4",
		  "css": "folder-empty",
		  "code": 61716,
		  "src": "fontawesome"
		},
		{
		  "uid": "6533bdc16ab201eb3f3b27ce989cab33",
		  "css": "folder-open-empty",
		  "code": 61717,
		  "src": "fontawesome"
		},
		{
		  "uid": "6975c4c656d41a0283d9c52516bf38ed",
		  "css": "delicious",
		  "code": 61861,
		  "src": "fontawesome"
		},
		{
		  "uid": "347c38a8b96a509270fdcabc951e7571",
		  "css": "database",
		  "code": 61888,
		  "src": "fontawesome"
		},
		{
		  "uid": "872d9516df93eb6b776cc4d94bd97dac",
		  "css": "video",
		  "code": 59419,
		  "src": "fontawesome"
		},
		{
		  "uid": "381da2c2f7fd51f8de877c044d7f439d",
		  "css": "picture",
		  "code": 59420,
		  "src": "fontawesome"
		},
		{
		  "uid": "3c961c1a8d874815856fc6637dc5a13c",
		  "css": "file-image",
		  "code": 61893,
		  "src": "fontawesome"
		},
		{
		  "uid": "c08a1cde48d96cba21d8c05fa7d7feb1",
		  "css": "doc-text-inv",
		  "code": 61788,
		  "src": "fontawesome"
		},
		{
		  "uid": "178053298e3e5b03551d754d4b9acd8b",
		  "css": "doc-inv",
		  "code": 61787,
		  "src": "fontawesome"
		},
		{
		  "uid": "5408be43f7c42bccee419c6be53fdef5",
		  "css": "doc-text",
		  "code": 61686,
		  "src": "fontawesome"
		},
		{
		  "uid": "c8585e1e5b0467f28b70bce765d5840c",
		  "css": "docs",
		  "code": 61637,
		  "src": "fontawesome"
		},
		{
		  "uid": "1b5a5d7b7e3c71437f5a26befdd045ed",
		  "css": "doc",
		  "code": 59421,
		  "src": "fontawesome"
		},
		{
		  "uid": "f48ae54adfb27d8ada53d0fd9e34ee10",
		  "css": "trash-empty",
		  "code": 59422,
		  "src": "fontawesome"
		},
		{
		  "uid": "bbfb51903f40597f0b70fd75bc7b5cac",
		  "css": "trash",
		  "code": 61944,
		  "src": "fontawesome"
		},
		{
		  "uid": "e80ae555c1413a4ec18b33fb348b4049",
		  "css": "file-archive",
		  "code": 61894,
		  "src": "fontawesome"
		},
		{
		  "uid": "81db033e704eb7c586a365559d7c0f36",
		  "css": "file-audio",
		  "code": 61895,
		  "src": "fontawesome"
		},
		{
		  "uid": "dd69d9aa589ea7bc0a82a3fe67039f4b",
		  "css": "file-video",
		  "code": 61896,
		  "src": "fontawesome"
		},
		{
		  "uid": "26613a2e6bc41593c54bead46f8c8ee3",
		  "css": "file-code",
		  "code": 61897,
		  "src": "fontawesome"
		},
		{
		  "uid": "7aa8cc552d9d6d19815750fc339cf090",
		  "css": "object-group",
		  "code": 62023,
		  "src": "fontawesome"
		},
		{
		  "uid": "d4edf8563d5cb3b5f654808aa4fe438a",
		  "css": "object-ungroup",
		  "code": 62024,
		  "src": "fontawesome"
		},
		{
		  "uid": "c116c18983825a9b32a23a62fd18e1e6",
		  "css": "sticky-note",
		  "code": 62025,
		  "src": "fontawesome"
		},
		{
		  "uid": "b9ca73befd583b2197a56a68045c9c0c",
		  "css": "sticky-note-o",
		  "code": 62026,
		  "src": "fontawesome"
		},
		{
		  "uid": "12a80769cc7ac08e986deeafbe553054",
		  "css": "mouse-pointer",
		  "code": 62021,
		  "src": "fontawesome"
		},
		{
		  "uid": "9396b2d8849e0213a0f11c5fd7fcc522",
		  "css": "tasks",
		  "code": 61614,
		  "src": "fontawesome"
		}
	  ]
	}
	)";

}
