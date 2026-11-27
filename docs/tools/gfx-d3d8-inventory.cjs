// SPDX-License-Identifier: GPL-3.0-or-later
'use strict';

const { execFileSync } = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');

const baseline = '3d3ebe7c62d3c7113e0e59f155415818927a879a';
const sdkRevision = '7bddff8c01f5fb931c3cb73d4aa8e66d303d97bc';
const games = ['g_generals', 'z_generals', 'g_worldbuilder', 'z_worldbuilder'];
const roots = ['Core', 'Generals', 'GeneralsMD', 'Dependencies', 'cmake', 'CMakeLists.txt'];
const sourceFile = /\.(?:h|hh|hpp|hxx|c|cc|cpp|cxx|inl|ipp|rc|cmake)$|(?:^|\/)CMakeLists\.txt$/i;
const headerFile = /\.(?:h|hh|hpp|hxx|inl|ipp)$/i;
const buildFile = /(?:^|\/)CMakeLists\.txt$|\.cmake$/i;
const outside = file => file.split('/').includes('Test Builds');
const cursor = /^(?:ShowCursor|SetCursorPosition|SetCursorProperties)$/;
const slash = value => value.replace(/\\/g, '/');
const tsv = value => String(value ?? '').replace(/\t/g, '\\t').replace(/\r/g, '\\r');
const orderBy = (a, b) => a < b ? -1 : a > b ? 1 : 0;

function git(cwd, args, options = {}) {
    return execFileSync('git', args, { cwd, encoding: 'utf8', maxBuffer: 160 * 1024 * 1024, ...options });
}

// Strip comments while preserving offsets and newlines. Keep string literals only
// for include directives and build configuration.
function mask(source, keepStrings = false) {
    const tokens = /\/\/[^\r\n]*|\/\*[\s\S]*?(?:\*\/|$)|R"([^()\s]{0,16})\([\s\S]*?\)\1"|"(?:\\[\s\S]|[^"\\\r\n])*"|'(?:\\[\s\S]|[^'\\\r\n])*'/g;
    return source.replace(tokens, token => {
        if (keepStrings && !token.startsWith('//') && !token.startsWith('/*'))
            return token;
        return token.replace(/[^\r\n]/g, ' ');
    });
}

function readSdk(source) {
    const interfaces = new Map();
    const aliases = new Map();
    for (const match of source.matchAll(/DECLARE_INTERFACE_\((IDirect3D\w+),\s*(\w+)\)\s*\{([\s\S]*?)\n\s*\};/g)) {
        const [, name, parent, declarations] = match;
        const methods = new Set([...declarations.matchAll(/STDMETHOD\(\s*(\w+)\s*\)|STDMETHOD_\(\s*[^,]+,\s*(\w+)\s*\)/g)]
            .map(method => method[1] || method[2]));
        interfaces.set(name, { parent, methods });
        aliases.set(name, name);
    }
    for (const match of source.matchAll(/typedef\s+struct\s+(IDirect3D\w+)\s+([^;]+);/g))
        for (const alias of match[2].matchAll(/\*\s*(\w+)/g))
            if (interfaces.has(match[1])) aliases.set(alias[1], match[1]);
    return { interfaces, aliases };
}

function register(map, key, value) {
    if (!map.has(key)) map.set(key, new Set());
    map.get(key).add(value);
}

const getters = new Map([
    ['_Get_D3D_Device8', 'IDirect3DDevice8'], ['Get_D3D_Device8', 'IDirect3DDevice8'],
    ['_Get_D3D8', 'IDirect3D8'], ['Get_D3D8', 'IDirect3D8'],
    ['Peek_D3D_Surface', 'IDirect3DSurface8'], ['Get_D3D_Surface_Level', 'IDirect3DSurface8'],
    ['Get_DX8_Vertex_Buffer', 'IDirect3DVertexBuffer8'], ['Get_DX8_Index_Buffer', 'IDirect3DIndexBuffer8'],
    ['Peek_D3D_Texture', 'IDirect3DTexture8'], ['Peek_D3D_Base_Texture', 'IDirect3DBaseTexture8'],
    ['Peek_D3D_Cube_Texture', 'IDirect3DCubeTexture8'], ['Peek_D3D_Volume_Texture', 'IDirect3DVolumeTexture8']
]);

function collectTypes(source, sdk) {
    const aliases = new Map(sdk.aliases);
    for (const [, oldType, alias, usingAlias, usingType] of source.matchAll(
        /\btypedef\s+(?:struct\s+)?(\w+)\s*\*?\s+(\w+)\s*;|\busing\s+(\w+)\s*=\s*(\w+)\s*\*?\s*;/g)) {
        const actual = aliases.get(oldType || usingType);
        if (actual) aliases.set(alias || usingAlias, actual);
    }
    const receivers = new Map();
    const types = [...aliases.keys()].join('|');
    for (const [, type, receiver] of source.matchAll(new RegExp(
        '\\b(' + types + ')\\s*(?:const\\s+)?(?:\\*+\\s*|\\s+)(\\w+)\\b', 'g')))
        register(receivers, receiver, aliases.get(type));
    for (const [, receiver, getter] of source.matchAll(/\b(\w+)\s*=\s*(?:\w+::)?(\w+)\s*\(/g))
        if (getters.has(getter)) register(receivers, receiver, getters.get(getter));
    return receivers;
}

function methodTypes(sdk) {
    const methods = new Map();
    for (const [type, entry] of sdk.interfaces)
        for (const method of entry.methods) register(methods, method, type);
    return methods;
}

function nativeCalls(source, receivers, memberTypes, sdk, methods) {
    const calls = [];
    function add(method, index, types, evidence) {
        if (!methods.has(method)) return;
        if (types.size === 0 && !cursor.test(method) &&
            /^(?:AddRef|Release|GetType|GetDevice|Lock|Unlock|Reset|GetDesc|GetData|SetPrivateData|GetPrivateData|FreePrivateData)$/.test(method))
            return;
        const plausible = [...types].filter(type => {
            for (let current = type; sdk.interfaces.has(current); current = sdk.interfaces.get(current).parent)
                if (sdk.interfaces.get(current).methods.has(method)) return true;
            return false;
        });
        calls.push({ method, index, type: plausible.length === 1 ? plausible[0] : '?',
            evidence: plausible.length === 1 ? evidence : 'SDK name, receiver unresolved' });
    }
    for (const match of source.matchAll(/(?:\b(\w+(?:\[[^\]\r\n]*\])?)|\b(\w+)\s*\([^;{}]*?\))?\s*->\s*(\w+)\s*\(/g)) {
        const receiver = match[1]?.split('[')[0];
        const getterType = getters.get(match[2]);
        const inferred = receiver && (receivers.get(receiver) || memberTypes.get(receiver));
        const receiverTypes = getterType ? new Set([getterType]) : inferred || new Set();
        const index = match.index + match[0].lastIndexOf(match[3]);
        add(match[3], index, receiverTypes, getterType ? 'typed getter' : 'typed receiver');
    }
    for (const match of source.matchAll(/\b(IDirect3D\w+?)_(\w+)\s*\(|\b(DX8CALL(?:_D3D|_HRES)?)\s*\(\s*(\w+)\s*\(/g)) {
        const macroType = match[3] === 'DX8CALL_D3D' ? 'IDirect3D8' : 'IDirect3DDevice8';
        const method = match[2] || match[4];
        const index = match.index + match[0].lastIndexOf(method);
        add(method, index, new Set([match[1] || macroType]), 'SDK or wrapper macro');
    }
    return calls;
}

const patterns = [
    ['include', /^\s*#\s*include\s*[<"][^>"]*\b(?:d3d8|d3d8types|d3dx8\w*|dx8wrapper|dx8vertexbuffer|dx8indexbuffer|dx8fvf|surfaceclass|dx8texman|ww3dformat)\.h\b/i],
    ['build', /\b(?:d3d8lib|d3d8|min-dx8-sdk|Direct3DCreate8)\b/gi],
    ['native', /\b(?:I|LP|P)DIRECT3D\w*8\b|\b(?:_?Get_D3D(?:_Device8|8)|DX8CALL(?:_D3D|_HRES)?)\b/gi],
    ['vertex layout', /\b(?:D3DFVF_\w+|DX8_FVF_\w+|FVFInfoClass|Get_FVF_\w+|VertexFormat(?:XYZ\w*|[A-Z]\w*))\b/g],
    ['index format', /\b(?:D3DFMT_INDEX(?:16|32)|IndexFormat)\b/g],
    ['depth format', /\bD3DFMT_D(?:15|16|24|32)\w*\b/g],
    ['texture format', /\b(?:D3DFMT_(?!INDEX(?:16|32)\b|D(?:15|16|24|32)\w*\b)\w+|WW3D_FORMAT_\w+)\b/g],
    ['format conversion', /\b(?:D3DFORMAT|WW3DZFormat|WW3DFormat|Convert_(?:D3D|WW3D)\w*Format)\b/g],
    ['buffer access', /\b(?:(?:DX8|Sorting)(?:Vertex|Index)BufferClass|Dynamic[VI]BAccessClass|Get_DX8_(?:Vertex|Index)_Buffer|Set_(?:Vertex|Index)_Buffer(?:_Index_Offset)?|DrawIndexedPrimitive)\b/g],
    ['map', /\b(?:D3DLOCK(?:ED)?_\w+|WriteLockClass|AppendLockClass|LockRect|UnlockRect|LockBox|UnlockBox|Lock_Surfaces|Unlock_Surfaces)\b|->\s*(?:Lock|Unlock)\s*\(/g],
    ['resource intent', /\b(?:D3DPOOL(?:_\w+)?|D3DUSAGE_\w+)\b/g],
    ['resource ref', /\bREF_PTR_(?:SET|RELEASE)\s*\(\s*\w*(?:[Tt]exture|[Ss]urface|[Bb]uffer|[Ss]corch)\w*|\b\w*(?:Surface|Texture|Buffer|SwapChain|Device)\w*->(?:AddRef|Release)\s*\(/g],
    ['texture', /\b(?:\w*TextureLoadTaskClass|DX8TextureManagerClass|D3D(?:SURFACE|VOLUME)_DESC|(?:Get|Peek|Set)_D3D_(?:Base_)?(?:Cube_|Volume_)?(?:Texture|Surface)(?:_Level)?|(?:Get|Acquire)_(?:Surface|Volume)_Level|Convert_Texture|D3DX(?:Create|Load|Save)\w*(?:Texture|Surface)\w*)\b/g],
    ['target', /\b(?:D3DPRESENT_PARAMETERS|Set_Render_Target(?:_With_Z)?|Get_Render_Target|Create_Render_Target|Get_D3D_Back_Buffer|saveSurface)\b/g],
    ['recovery', /\b(?:Reset_Device|Release_Device|SetCleanupHook|DX8_CleanupHook|ReleaseResources|ReAcquireResources|Invalidate_Cached_Render_States|(?:Recreate|Release)_Textures|D3DERR_DEVICE(?:LOST|NOTRESET))\b/g],
    ['state and shader', /\b(?:D3DRS_\w+|D3DTSS_\w+|D3DTA_\w+|D3DTOP_\w+|D3DPT_\w+)\b/g],
    ['creation', /\b(?:Create(?:_DX8_|_D3D_|_)?(?:Vertex_?Buffer|Index_?Buffer|Cube_?Texture|Volume_?Texture|Texture|Image_?Surface|Depth_?Stencil_?Surface|Additional_?Swap_?Chain))\b/g]
];

function classify(file, code, symbols, calls, categories) {
    const ops = symbols.join(' ');
    const phase = (primary, secondary = '') => ({ primary, secondary });
    if (/\/WWAudio\/|\/Babylon\/|\/Win32DIMouse\.cpp$/i.test(file) &&
        !calls.some(call => call.type !== '?') && !categories.includes('native'))
        return phase('outside graphics');
    if (buildFile.test(file)) return phase('13b.5', '13c.1');
    if (calls.some(call => cursor.test(call.method))) return phase('13b.4', '13b.3,13b.5,13c.1');
    if (categories.includes('index format') || categories.includes('vertex layout')) return phase('13b.2a');
    if (categories.includes('depth format')) return phase('13b.4');
    if (categories.includes('texture format')) return phase('13b.3');
    if (categories.includes('format conversion')) return phase('13b.3', '13b.2a,13b.4');
    if (categories.includes('target') || categories.includes('recovery') ||
        /\b(?:GetBackBuffer|Present|TestCooperativeLevel|Reset|SetViewport|GetViewport|SetGammaRamp|GetGammaRamp|BeginScene|EndScene|CopyRects|GetFrontBuffer|GetRenderTarget|SetRenderTarget)\b/.test(ops))
        return phase('13b.4', '13c.1');
    if (categories.includes('state and shader') || /\b(?:\w+Shader\w*|SetRenderState|SetTextureStageState)\b/.test(ops))
        return phase('13d');
    if (categories.includes('buffer access') || /\b(?:\w+Buffer|SetIndices|SetStreamSource)\b/.test(ops) ||
        (/\/(?:dx8(?:vertex|index)buffer|W3DScorch|W3DRoadBuffer|W3DVolumetricShadow|W3DTerrainParticles|W3DTerrainTracks)\.cpp$/i.test(file) &&
         categories.some(name => ['resource intent', 'resource ref', 'map', 'creation'].includes(name)))) {
        if (/\b(?:(?:D3D)?USAGE_DYNAMIC|D3DLOCK_(?:DISCARD|NOOVERWRITE)|Dynamic[VI]BAccessClass)\b/.test(code) ||
            /(?:VolumetricShadow|TerrainParticles)\.cpp$/.test(file))
            return phase('13b.2c');
        return /dx8(?:index|vertex)buffer\.cpp$/i.test(file) ? phase('13b.2b', '13b.2c') : phase('13b.2b');
    }
    if (categories.includes('texture') ||
        /\b(?:\w*Texture\w*|\w*Surface\w*|LockRect|LockBox|UnlockRect|UnlockBox)\b/.test(ops))
        return phase('13b.3');
    if (/\b(?:CreateDevice|GetDeviceCaps|CheckDevice\w*|GetAdapter\w*)\b/.test(ops))
        return phase('13c.1', '13b.5');
    return phase('13b.5');
}

function lineNumbers(text) {
    const starts = [0];
    let offset = text.indexOf('\n');
    while (offset >= 0) {
        starts.push(offset + 1);
        offset = text.indexOf('\n', offset + 1);
    }
    return index => {
        let lo = 0, hi = starts.length;
        while (hi - lo > 1) {
            const middle = (lo + hi) >> 1;
            if (index < starts[middle]) hi = middle;
            else lo = middle;
        }
        return lo;
    };
}

function describeLine(code, context, file, calls) {
    if (buildFile.test(file)) return 'build reference';
    if (/^\s*#/.test(code)) return 'directive';
    if (calls.length) return 'call or wrapper macro';
    if (/^\s*(?:[\w:&*<>~]+\s+)*\w+(?:::\w+)+\s*\(/.test(code) && !context)
        return 'definition signature';
    if (!context && /^\s*(?:using|typedef|class|struct|enum|extern|static|constexpr)\b/.test(code))
        return 'declaration or file scope';
    return context ? 'function body' : 'context unresolved or file scope';
}

const operations = [
    ['create', /\b(?:Create\w+|allocateBuffers|NEW_REF)\b/],
    ['map or write', /\b(?:Lock\w*|WriteLockClass|AppendLockClass|updateScorches|loadRoadsInVertexAndIndexBuffers)\b/],
    ['unmap', /\bUnlock\w*\b/],
    ['bind', /\b(?:Set_(?:Vertex|Index)_Buffer\w*|SetStreamSource|SetIndices|SetTexture|SetRenderTarget)\b/],
    ['draw', /\b(?:Draw\w*Primitive\w*|Draw_Triangles)\b/],
    ['retain or release', /\b(?:AddRef|Release|REF_PTR_(?:SET|RELEASE)|freeBuffers)\b/],
    ['recreate', /\b(?:Reset\w*|ReAcquireResources|Recreate_Textures|invalidateBuffers)\b/]
];

function scan(file, source, sdk, methods, members, targetData) {
    const commentsMasked = mask(source, true);
    const lexical = mask(source);
    const lineIndex = lineNumbers(lexical);
    const callMap = new Map();
    for (const call of nativeCalls(lexical, collectTypes(lexical, sdk), members, sdk, methods)) {
        const i = lineIndex(call.index);
        if (!callMap.has(i)) callMap.set(i, []);
        callMap.get(i).push(call);
    }
    const guards = [];
    const raw = source.split(/\r?\n/);
    const lines = lexical.split(/\r?\n/);
    const commented = commentsMasked.split(/\r?\n/);
    const rows = [];
    const owners = targetData.owners.get(file.toLowerCase()) || new Set();
    const targets = targetData.membership.get(file.toLowerCase()) || new Set();
    let fn = '';
    let depth = 0;
    let pending = '';
    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        const pragma = line.match(/^\s*#\s*(if|ifdef|ifndef|else|elif|endif)\b\s*(.*)/);
        if (pragma) {
            const [, directive, expression] = pragma;
            if (['if', 'ifdef', 'ifndef'].includes(directive))
                guards.push({ expression: directive + ' ' + expression.trim(), off: directive === 'if' && expression.trim() === '0' });
            else if (directive === 'endif') guards.pop();
            else if (guards.length) {
                const prev = guards[guards.length - 1];
                guards[guards.length - 1] = { expression: directive + ' after ' + prev.expression, off: false };
            }
        }
        const definition = depth === 0 && line.match(/^\s*(?:[\w:*&<>~,]+\s+)*((?:\w+::)+(?:~?\w+))\s*\(/);
        if (definition && !line.includes(';')) pending = definition[1];
        if (pending && line.includes('{')) {
            fn = pending;
            pending = '';
        }
        if (!pragma) depth += (line.match(/\{/g)?.length || 0) - (line.match(/\}/g)?.length || 0);
        const calls = callMap.get(i) || [];
        const poolEntry = /\/GameMemoryInitPools_\w+\.inl$/.test(file) && /^\s*\{\s*"/.test(commented[i]);
        const tested = buildFile.test(file) || poolEntry || /^\s*#\s*(?:include|pragma)/.test(line) ? commented[i] : line;
        const hits = patterns.map(([kind, re]) => [kind, tested.match(re) || []]).filter(hit => hit[1].length);
        if (calls.length || hits.length) {
            const symbols = [...new Set([...hits.flatMap(hit => hit[1]), ...calls.map(call => call.method)])];
            const categories = hits.map(hit => hit[0]);
            if (calls.length) categories.push('SDK method');
            const scope = guards.some(guard => guard.off) ? 'excluded #if 0' :
                buildFile.test(file) ? 'build configuration' :
                targets.size ? 'CMake target' : headerFile.test(file) ? 'header outside target lists' : 'outside selected targets';
            const phases = classify(file, line, symbols, calls, categories);
            const dependency = phases.primary === 'outside graphics' ? 'non-graphics match' :
                calls.some(call => call.type !== '?') ? 'typed native call' :
                calls.length ? 'candidate SDK method' :
                /^\s*Renderer::/.test(line) ? 'renderer handoff' : 'native token or related operation';
            rows.push({
                path: file, line: i + 1, scope, targets: [...targets].sort(orderBy).join(','),
                owner: [...owners].sort(orderBy).join(','), context: fn || 'unresolved or file scope',
                kind: poolEntry ? 'memory pool configuration' : describeLine(line, fn, file, calls), dependency,
                native_evidence: [...new Set(calls.map(call => call.type + '::' + call.method + ' [' + call.evidence + ']'))].join(', '),
                guards: guards.map(guard => guard.expression).join(' && '), category: categories.join(','),
                operation: operations.filter(([, re]) => re.test(line) || re.test(symbols.join(' '))).map(([name]) => name).join(','),
                primary_phase: phases.primary, secondary_phases: phases.secondary,
                symbols: symbols.join(', '), source: raw[i]
            });
        }
        if (depth === 0 && line.includes('}')) fn = '';
    }
    return rows;
}

function loadGraph(graph) {
    const file = fs.readdirSync(graph).filter(name => /^codemodel-.*\.json$/.test(name)).sort(orderBy).at(-1);
    if (!file) throw Error('CMake codemodel-v2 reply is missing');
    const json = name => JSON.parse(fs.readFileSync(path.join(graph, name), 'utf8'));
    const model = json(file);
    const release = model.configurations.find(config => config.name === 'Release');
    if (!release) throw Error('CMake Release configuration is missing');
    const items = new Map(release.targets.map(item => [item.id, json(item.jsonFile)]));
    const membership = new Map(), owners = new Map(), definitions = {};
    for (const game of games) {
        const root = release.targets.find(target => target.name === game);
        if (!root) throw Error('CMake target is missing: ' + game);
        const flags = items.get(root.id).compileGroups.filter(group => group.language === 'CXX')
            .flatMap(group => group.defines.map(value => value.define))
            .filter(flag => /^(?:BUILD_WITH_D3D8|RTS_RELEASE|RTS_ZEROHOUR)(?:=|$)/.test(flag));
        definitions[game] = [...new Set(flags)].sort(orderBy);
        if (!definitions[game].includes('BUILD_WITH_D3D8') || !definitions[game].includes('RTS_RELEASE') ||
            definitions[game].includes('RTS_ZEROHOUR=1') !== game.startsWith('z_'))
            throw Error('Unexpected Win32 Release renderer or edition definitions for ' + game);
        const pending = [root.id], visited = new Set();
        while (pending.length) {
            const id = pending.pop();
            if (visited.has(id) || !items.has(id)) continue;
            visited.add(id);
            const item = items.get(id);
            for (const source of item.sources ?? []) {
                let name = slash(source.path);
                if (path.isAbsolute(name)) name = slash(path.relative(model.paths.source, name));
                if (source.isGenerated || name.startsWith('../') || outside(name)) continue;
                register(membership, name.toLowerCase(), game);
                register(owners, name.toLowerCase(), item.name);
            }
            for (const dep of item.dependencies ?? []) pending.push(dep.id);
        }
    }
    return { membership, owners, definitions };
}

function loadBaseline(repo) {
    const files = git(repo, ['ls-tree', '-rz', baseline, '--', ...roots]).split('\0').filter(Boolean)
        .map(entry => entry.match(/^\d+ blob ([a-f0-9]+)\t([\s\S]+)$/))
        .filter(entry => entry && sourceFile.test(entry[2]) && !outside(entry[2]))
        .map(entry => ({ sha: entry[1], file: entry[2] })).sort((a,b) => orderBy(a.file,b.file));
    const input = Buffer.from(files.map(file => file.sha).join('\n') + '\n');
    const stream = git(repo, ['cat-file', '--batch'], { input, encoding: null });
    let start = 0;
    for (const file of files) {
        const next = stream.indexOf(10, start);
        const [, sha, size] = /^([a-f0-9]+) blob (\d+)$/.exec(stream.toString('ascii', start, next)) || [];
        if (sha !== file.sha || !size) throw Error('Unexpected blob header for ' + file.file);
        const end = next + 1 + Number(size);
        file.source = stream.toString('utf8', next + 1, end);
        if (stream[end] !== 10) throw Error('Invalid blob boundary for ' + file.file);
        start = end + 1;
    }
    if (start !== stream.length) throw Error('Unconsumed Git output');
    return files;
}

function counts(rows, key) {
    const tally = new Map();
    for (const row of rows) tally.set(row[key], (tally.get(row[key]) || 0) + 1);
    return Object.fromEntries([...tally.entries()].sort((a,b) => orderBy(a[0],b[0])));
}

function report(stats) {
    const rows = Object.entries(stats.scopes).map(([scope, n]) => '| ' + scope + ' | ' + n + ' |').join('\n');
    const targetRows = Object.entries(stats.targets).map(([name, count]) =>
        '| ' + name + ' | ' + count.files + ' | ' + count.rows + ' |').join('\n');
    const phaseRows = Object.entries(stats.phases).map(([name, count]) => '| ' + name + ' | ' + count + ' |').join('\n');
    return [
        '# GFX 13b.1b — Direct3D 8 resource inventory',
        '',
        'Source baseline **' + baseline + '**. The DX8 SDK checkout is pinned at **' + sdkRevision + '**. The [resource contract](gfx-resource-contract.md) and [review](gfx-d3d8-resource-inventory-review.md) explain the migration and its acceptance criteria.',
        '',
        'Examined **' + stats.filesScanned + '** Git blobs. The [TSV](gfx-d3d8-resource-inventory.tsv) records **' + stats.rows + '** unique path/line pairs in **' + stats.filesMatched + '** files. Every source field preserves the baseline line, with tabs escaped. Each row has one primary phase and can name secondary phases.',
        '',
        '| Scope | Rows |', '| --- | ---: |', rows, '',
        '| Win32 Release target | Files | Rows with CMake target scope |', '| --- | ---: | ---: |', targetRows, '',
        'Target rows overlap through shared libraries. The Release CMake codemodel supplies file and target ownership. A listed source can contain optional or inactive code. The guards column preserves those conditions and labels literal #if 0. CMake configuration files and linked SDK declarations supply build evidence.',
        '',
        'The C++ compile groups for all four executable targets define BUILD_WITH_D3D8 and RTS_RELEASE. The two Zero Hour targets define RTS_ZEROHOUR=1. The Generals executable targets omit RTS_ZEROHOUR. These flags identify the audited configuration. Conditions on optional features remain visible for review.',
        '',
        '| Primary phase | Candidate lines |', '| --- | ---: |', phaseRows, '',
        '## Native calls and exceptions',
        '',
        '- The scanner reads interfaces, aliases and method names from the pinned SDK. Call detection covers typed local declarations, member names, direct getters, COM macros and multiline calls. Type resolution is lexical. Entries with unresolved receiver types remain explicit candidates for source review.',
        '- [W3DMouse.cpp](../Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DMouse.cpp) has seven confirmed device call sites for ShowCursor, SetCursorPosition and SetCursorProperties. They go to 13b.4 with dependencies on texture surfaces, native-handle audit and backend bring-up. Cursor creation, hotspot, animation, fullscreen updates, thread ownership and reset need separate checks before migrating this path.',
        '- The header scope outside the four target lists retains WWAudio buffer references, Babylon string locks, W3DView texture references and WOLAPI declarations that match generic SDK method names. WWAudio and Babylon entries classified as non-graphics are excluded from migration. W3DView and WOL utilities require a target-graph check if brought into scope. CMake source membership and the lexical scanner do not establish runtime reachability.',
        '- The device and SDK blobs are inputs to the scan. Generated code and third-party SDK implementation are outside the first-party source inventory. The register captures optional clauses and unresolved context for follow-up.',
        '',
        '## Buffer inputs for 13b.2a',
        '',
        '| Consumers | Producer and ownership | Phase |', '| --- | --- | --- |',
        '| [Scorch](../Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DScorch.cpp) | VertexFormatXYZDUV1, 24-byte stride, position at 0, packed color at 12, UV at 16. UInt16 indices. Own buffers, rewrite them and retain the existing invalidation and draw order. | 13b.2a layout, 13b.2b non-streaming adapter |',
        '| Roads in [Generals](../Generals/Code/GameEngineDevice/Source/W3DDevice/GameClient/W3DRoadBuffer.cpp) and [Zero Hour](../GeneralsMD/Code/GameEngineDevice/Source/W3DDevice/GameClient/W3DRoadBuffer.cpp) | Same vertex layout and UInt16 index width as scorch. Each road owner allocates, locks, binds and releases its buffers. Generals uses dynamic usage for its road vertex buffer, and Zero Hour selects usage per road path. | 13b.2b and 13b.2c according to usage |',
        '| [Terrain particles](../Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DTerrainParticles.cpp) and [sorting buffers](../Core/Libraries/Source/WWVegas/WW3D2/dx8vertexbuffer.h) | VertexFormatXYZNDUV2, 44-byte stride, position 0, normal 12, packed color 24, UVs 28 and 36. Dynamic GPU reservations and CPU sorting pages need distinct lifetime checks. | 13b.2a layout, 13b.2c mapping and lifetime |',
        '| [General FVF layouts](../Core/Libraries/Source/WWVegas/WW3D2/dx8fvf.h) and [DX8 draw binding](../Core/Libraries/Source/WWVegas/WW3D2/dx8wrapper.cpp) | Validate stride and field offsets against each producer. Byte offsets are for maps. Base vertex is vba_offset + index_base_offset, first index is iba_offset + start_index. TG3 and DMAP require separate compatibility decisions. | 13b.2a |',
        '',
        'The Win32 validation for #88 compared 11 ordinary named layouts to the actual structures. The TG3 structure matches the native declaration and differs from legacy metadata. DMAP occupies 48 bytes in the structure and 52 bytes in the native declaration. Keep shader-specific, transformed or mutable legacy metadata behind caller-specific verification before migration.',
        '',
        '## Reproduce',
        '',
        'Use a Win32 CMake Release configuration of the pinned baseline or a code-equivalent checkout with both games and both World Builders enabled. Request codemodel-v2 under the build directory’s .cmake/api/v1/query, then run the configure step. Use the pinned SDK checkout referenced by cmake/dx8.cmake.',
        '',
        'From the source checkout run node docs/tools/gfx-d3d8-inventory.cjs . <CMake-reply-directory> <SDK-checkout> <output-directory>. Run node docs/tools/gfx-d3d8-inventory.cjs --self-test for the scanner regression fixtures. An optional fifth argument supplies the previous TSV for a before/after delta written to standard output.',
        '',
        'The schema records path and line, source text, scope, CMake targets and owners, inferred context, kind, dependency, SDK receiver evidence, guards, categories, resource operations, primary and secondary phase, and symbols.'
    ].join('\n') + '\n';
}

function selfTest() {
    const assert = require('node:assert/strict');
    const sdk = readSdk(
        'DECLARE_INTERFACE_(IDirect3DDevice8, IUnknown)\n{\nSTDMETHOD_(BOOL,ShowCursor)(THIS) PURE;\n' +
        'STDMETHOD_(void,SetCursorPosition)(THIS) PURE;\nSTDMETHOD(SetCursorProperties)(THIS) PURE;\n};\n' +
        'typedef struct IDirect3DDevice8 *LPDIRECT3DDEVICE8;\n');
    const source = 'using LocalDevice = LPDIRECT3DDEVICE8;\nLocalDevice device;\n' +
        'void Renderer::Run()\n{\n#if 0\ndevice ->\n  ShowCursor(true);\n#endif\n' +
        'device->SetCursorPosition(0,0,0);\nDX8CALL(SetCursorProperties(0,0,s));\n' +
        '// device->ShowCursor(false);\nconst char* x = "device->ShowCursor(false)";\n}\n';
    const rows = scan('Core/Test.cpp', source, sdk, methodTypes(sdk), new Map(), { owners: new Map(), membership: new Map() });
    const natives = rows.filter(row => row.native_evidence);
    assert.deepEqual(natives.map(row => row.line), [7, 9, 10]);
    assert.equal(natives[0].scope, 'excluded #if 0');
    assert.equal(natives[1].native_evidence, 'IDirect3DDevice8::SetCursorPosition [typed receiver]');
    assert.equal(natives[1].context, 'Renderer::Run');
    assert.equal(natives[2].kind, 'call or wrapper macro');
    const noise = scan('Core/Tools/Babylon/TransDB.h', 'void LockText() { text->Lock(); };\n', sdk,
        methodTypes(sdk), new Map(), { owners: new Map(), membership: new Map() });
    assert.equal(noise[0].dependency, 'non-graphics match');
    assert.equal(classify('Core/x.cpp','', ['D3DFMT_INDEX16'], [], ['index format']).primary, '13b.2a');
    assert.equal(classify('Core/x.cpp','', ['D3DFMT_D24S8'], [], ['depth format']).primary, '13b.4');
    console.log('Inventory scanner regression fixtures passed.');
}

function main(args) {
    if (args[0] === '--self-test') return selfTest();
    if (args.length < 4 || args.length > 5) throw Error('Provide repo, CMake reply, SDK checkout, output directory and optional previous TSV');
    const [repo, graph, sdkDir, dest, previous] = args;
    if (git(sdkDir, ['rev-parse','HEAD']).trim() !== sdkRevision) throw Error('Unexpected SDK revision');
    const sdk = readSdk(git(sdkDir, ['show', sdkRevision + ':d3d8.h']));
    if (sdk.interfaces.size < 10) throw Error('Could not parse pinned DX8 SDK interfaces');
    const methods = methodTypes(sdk);
    const targetData = loadGraph(graph);
    const files = loadBaseline(repo);
    const members = new Map();
    for (const file of files.filter(file => headerFile.test(file.file)))
        for (const [member, types] of collectTypes(mask(file.source), sdk))
            if (/^m_|^D3D/.test(member)) for (const type of types) register(members, member, type);
    const rows = files.flatMap(file => scan(file.file, file.source, sdk, methods, members, targetData));
    const key = row => row.path + ':' + row.line;
    const indexed = new Map(rows.map(row => [key(row), row]));
    if (indexed.size !== rows.length) throw Error('Duplicate path and line');
    const cursorRows = rows.filter(row => /\/W3DMouse\.cpp$/.test(row.path) && /IDirect3DDevice8::(?:ShowCursor|SetCursor)/.test(row.native_evidence));
    if (cursorRows.map(row => row.line).join(',') !== '115,399,419,420,492,501,514')
        throw Error('Cursor regression: ' + cursorRows.map(row => row.line).join(','));
    const stats = {
        baseline, sdkRevision, filesScanned: files.length, rows: rows.length,
        filesMatched: new Set(rows.map(row => row.path)).size, scopes: counts(rows, 'scope'),
        dependency: counts(rows, 'dependency'), phases: counts(rows, 'primary_phase'), definitions: targetData.definitions,
        targets: Object.fromEntries(games.map(name => {
            const subset = rows.filter(row => row.scope === 'CMake target' && row.targets.split(',').includes(name));
            return [name, { files: new Set(subset.map(row => row.path)).size, rows: subset.length }];
        }))
    };
    fs.mkdirSync(dest, { recursive: true });
    const columns = Object.keys(rows[0]);
    fs.writeFileSync(path.join(dest, 'gfx-d3d8-resource-inventory.tsv'),
        columns.join('\t') + '\n' + rows.map(row => columns.map(col => tsv(row[col])).join('\t')).join('\n') + '\n');
    fs.writeFileSync(path.join(dest, 'gfx-d3d8-resource-inventory.md'), report(stats));
    if (previous) {
        const [header, ...oldRows] = fs.readFileSync(previous, 'utf8').trimEnd().split(/\r?\n/).map(line => line.split('\t'));
        const oldPhase = header.indexOf('planned_area');
        const oldSource = header.indexOf('source');
        if (oldPhase < 0 || oldSource < 0) throw Error('Previous ledger schema is missing planned_area or source');
        const before = new Set(oldRows.map(row => row[0] + ':' + row[1]));
        const removed = oldRows.filter(row => !indexed.has(row[0] + ':' + row[1]));
        stats.delta = {
            previous: before.size,
            added: rows.filter(row => !before.has(key(row))).length,
            removed: removed.length,
            removedStringLiterals: removed.filter(row => row[oldSource].includes('"')).length,
            retained: rows.filter(row => before.has(key(row))).length,
            formerlyMixedAssignedPrimary: oldRows.filter(row => row[oldPhase] === '13b.2 buffers or layout, 13b.3 textures or surface'
                && indexed.has(row[0] + ':' + row[1])).length
        };
    }
    console.log(JSON.stringify(stats, null, 2));
}
main(process.argv.slice(2));
