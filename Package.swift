// swift-tools-version: 6.3

import PackageDescription

// What a bare ./configure builds: the sources libspeex/Makefile.am lists
// unconditionally, and smallft.c for the FFT it picks. Written out rather than
// walked, so Swift Package Manager leaves the rest of the repository — the
// other build systems, the ports, the documentation — alone.
let sources = [
    "bits.c",
    "cb_search.c",
    "exc_10_16_table.c",
    "exc_10_32_table.c",
    "exc_20_32_table.c",
    "exc_5_256_table.c",
    "exc_5_64_table.c",
    "exc_8_128_table.c",
    "filters.c",
    "gain_table.c",
    "gain_table_lbr.c",
    "hexc_10_32_table.c",
    "hexc_table.c",
    "high_lsp_tables.c",
    "lpc.c",
    "lsp.c",
    "lsp_tables_nb.c",
    "ltp.c",
    "modes.c",
    "modes_wb.c",
    "nb_celp.c",
    "quant_lsp.c",
    "sb_celp.c",
    "smallft.c",
    "speex.c",
    "speex_callbacks.c",
    "speex_header.c",
    "stereo.c",
    "vbr.c",
    "vq.c",
    "window.c",
].map { "libspeex/\($0)" }

let package = Package(
    name: "speex",
    products: [
        .library(name: "libspeex", targets: ["libspeex"]),
    ],
    targets: [
        // Named for the library rather than for the module the manager would
        // rather build: given a target called `speex`, it takes
        // include/speex/speex.h for an umbrella header and then rejects the
        // module for leaving its neighbours out. Any other name and it makes
        // an umbrella directory of include/, which is what the headers are.
        .target(
            name: "libspeex",
            path: ".",
            // Swift Package Manager still walks the target's directory looking
            // for resources, whatever `sources` says, and stops on the
            // localized ones in macosx/. Only directories are named: the list
            // is short and upstream rarely adds another, whereas the loose
            // files at the root come and go.
            exclude: [
                "contrib",
                "doc",
                "html",
                "m4",
                "macosx",
                "speexclient",
                "src",
                "symbian",
                "ti",
                "tmv",
                "win32",
            ],
            sources: sources,
            publicHeadersPath: "include",
            // What configure would have written into config.h. Passed as
            // flags instead, so this build adds no file that configure also
            // generates and nothing here can be clobbered by running it.
            //
            // On Apple platforms speex_types.h defines the sized types itself
            // (the `__APPLE__ && __MACH__` branch), so speex_config_types.h —
            // the other generated header — is not needed either.
            cSettings: [
                // Floating point, and the FFT configure pairs with it. For the
                // fixed-point codec: FIXED_POINT and USE_KISS_FFT instead.
                .define("FLOATING_POINT"),
                .define("USE_SMALLFT"),
                // Clang has variable-length arrays, so the codec needs neither
                // alloca nor a preallocated scratch area.
                .define("VAR_ARRAYS"),
                .define("EXPORT", to: ""),
            ]
        ),
    ]
)
