function plot_sp_export_pair(input_path)
% Interactive viewer for exported PC / MTD radar data.
% Features:
% - Open one exported PC or MTD file and auto-resolve its pair
% - Switch between PC / exported MTD / recomputed MTD / compare mode
% - Switch plot mode
% - Switch range-unit row
clc
clear all
close all

    if nargin < 1
        input_path = '';
    end

    state = struct();
    state.pc = [];
    state.mtd = [];
    state.mtd_from_pc = [];
    state.prefix = '';
    state.current_row = 1;

    fig = figure('Name', 'SignalCore Export Viewer', ...
                 'NumberTitle', 'off', ...
                 'MenuBar', 'none', ...
                 'Toolbar', 'none', ...
                 'Color', [0.96, 0.97, 0.99], ...
                 'Position', [80, 60, 1480, 860]);

    panel = uipanel('Parent', fig, ...
                    'Title', 'Control Panel', ...
                    'FontWeight', 'bold', ...
                    'BackgroundColor', [0.94, 0.95, 0.98], ...
                    'Position', [0.015, 0.04, 0.23, 0.92]);

    uicontrol(panel, 'Style', 'pushbutton', ...
                     'String', 'Open Export File', ...
                     'FontSize', 11, ...
                     'Position', [20, 740, 280, 36], ...
                     'Callback', @on_open_file);

    uicontrol(panel, 'Style', 'text', ...
                     'String', 'Data Source', ...
                     'HorizontalAlignment', 'left', ...
                     'BackgroundColor', get(panel, 'BackgroundColor'), ...
                     'Position', [20, 700, 280, 20]);
    source_popup = uicontrol(panel, 'Style', 'popupmenu', ...
                                    'String', {'PC', 'Exported MTD', 'Recomputed MTD from PC', 'Compare Recomputed vs Exported'}, ...
                                    'FontSize', 10, ...
                                    'Position', [20, 674, 280, 28], ...
                                    'Callback', @on_control_changed);

    uicontrol(panel, 'Style', 'text', ...
                     'String', 'Plot Mode', ...
                     'HorizontalAlignment', 'left', ...
                     'BackgroundColor', get(panel, 'BackgroundColor'), ...
                     'Position', [20, 634, 280, 20]);
    mode_popup = uicontrol(panel, 'Style', 'popupmenu', ...
                                  'String', {'Magnitude Image', 'I Image', 'Q Image', 'Magnitude Waveform', 'I Waveform', 'Q Waveform'}, ...
                                  'FontSize', 10, ...
                                  'Position', [20, 608, 280, 28], ...
                                  'Callback', @on_control_changed);

    uicontrol(panel, 'Style', 'text', ...
                     'String', 'Range Row', ...
                     'HorizontalAlignment', 'left', ...
                     'BackgroundColor', get(panel, 'BackgroundColor'), ...
                     'Position', [20, 568, 280, 20]);
    row_slider = uicontrol(panel, 'Style', 'slider', ...
                                  'Min', 1, 'Max', 1, 'Value', 1, ...
                                  'SliderStep', [1 1], ...
                                  'Position', [20, 544, 280, 24], ...
                                  'Callback', @on_row_slider);
    row_text = uicontrol(panel, 'Style', 'text', ...
                                'String', 'Row: 1 / 1', ...
                                'HorizontalAlignment', 'left', ...
                                'BackgroundColor', get(panel, 'BackgroundColor'), ...
                                'Position', [20, 518, 280, 20]);

    info_box = uicontrol(panel, 'Style', 'listbox', ...
                                'FontName', 'Consolas', ...
                                'FontSize', 10, ...
                                'BackgroundColor', [1, 1, 1], ...
                                'Position', [20, 20, 280, 480], ...
                                'String', {'No file loaded.'});

    ax1 = axes('Parent', fig, 'Position', [0.28, 0.57, 0.30, 0.34], 'Box', 'on');
    ax2 = axes('Parent', fig, 'Position', [0.64, 0.57, 0.30, 0.34], 'Box', 'on');
    ax3 = axes('Parent', fig, 'Position', [0.28, 0.10, 0.66, 0.34], 'Box', 'on');

    state.handles = struct('fig', fig, ...
                           'source_popup', source_popup, ...
                           'mode_popup', mode_popup, ...
                           'row_slider', row_slider, ...
                           'row_text', row_text, ...
                           'info_box', info_box, ...
                           'ax1', ax1, ...
                           'ax2', ax2, ...
                           'ax3', ax3);

    guidata(fig, state);

    if isempty(input_path)
        on_open_file();
    else
        load_files_into_state(input_path);
        redraw();
    end

    function on_open_file(~, ~)
        [file_name, file_path] = uigetfile({'*.dat', 'Exported DAT Files (*.dat)'}, ...
                                           'Select one exported PC or MTD file');
        if isequal(file_name, 0)
            return;
        end
        load_files_into_state(fullfile(file_path, file_name));
        redraw();
    end

    function load_files_into_state(selected_path)
        state = guidata(fig);
        [pc_file, mtd_file, prefix] = resolve_pair_files(selected_path);
        state.prefix = prefix;
        state.pc = [];
        state.mtd = [];
        state.mtd_from_pc = [];

        if ~isempty(pc_file)
            state.pc = load_export_dat(pc_file);
            state.mtd_from_pc = calc_mtd_from_pc(state.pc);
        end
        if ~isempty(mtd_file)
            state.mtd = load_export_dat(mtd_file);
        end

        max_rows = get_max_rows(state);
        state.current_row = min(max(1, state.current_row), max_rows);
        update_row_slider_limits(state);
        update_info_box(state);
        guidata(fig, state);
    end

    function on_control_changed(~, ~)
        redraw();
    end

    function on_row_slider(src, ~)
        state = guidata(fig);
        state.current_row = round(get(src, 'Value'));
        guidata(fig, state);
        redraw();
    end

    function update_row_slider_limits(state)
        max_rows = get_max_rows(state);
        if max_rows < 1
            max_rows = 1;
        end
        set(state.handles.row_slider, 'Min', 1, 'Max', max_rows, 'Value', min(state.current_row, max_rows));
        if max_rows == 1
            set(state.handles.row_slider, 'SliderStep', [1 1]);
        else
            set(state.handles.row_slider, 'SliderStep', [1/(max_rows-1), min(1, 5/(max_rows-1))]);
        end
        set(state.handles.row_text, 'String', sprintf('Row: %d / %d', min(state.current_row, max_rows), max_rows));
    end

    function n = get_max_rows(state)
        n = 1;
        candidates = {};
        if ~isempty(state.pc), candidates{end+1} = state.pc; end %#ok<AGROW>
        if ~isempty(state.mtd), candidates{end+1} = state.mtd; end %#ok<AGROW>
        if ~isempty(state.mtd_from_pc), candidates{end+1} = state.mtd_from_pc; end %#ok<AGROW>
        for k = 1:numel(candidates)
            n = max(n, candidates{k}.rows);
        end
    end

    function update_info_box(state)
        lines = {};
        lines{end+1} = ['Prefix: ', state.prefix];
        if ~isempty(state.pc)
            lines{end+1} = sprintf('PC: rows=%d cols=%d ch=%d', state.pc.rows, state.pc.cols, state.pc.channels);
            lines{end+1} = ['PC file: ', state.pc.file_path];
        else
            lines{end+1} = 'PC: not loaded';
        end
        if ~isempty(state.mtd)
            lines{end+1} = sprintf('Exported MTD: rows=%d cols=%d ch=%d', state.mtd.rows, state.mtd.cols, state.mtd.channels);
            lines{end+1} = ['MTD file: ', state.mtd.file_path];
        else
            lines{end+1} = 'Exported MTD: not loaded';
        end
        if ~isempty(state.mtd_from_pc)
            lines{end+1} = sprintf('Recomputed MTD: rows=%d cols=%d ch=%d', state.mtd_from_pc.rows, state.mtd_from_pc.cols, state.mtd_from_pc.channels);
        else
            lines{end+1} = 'Recomputed MTD: unavailable';
        end
        set(state.handles.info_box, 'String', lines, 'Value', 1);
    end

    function redraw()
        state = guidata(fig);
        update_row_slider_limits(state);

        source_idx = get(state.handles.source_popup, 'Value');
        mode_idx = get(state.handles.mode_popup, 'Value');
        current_row = max(1, round(get(state.handles.row_slider, 'Value')));
        state.current_row = current_row;
        guidata(fig, state);

        cla(state.handles.ax1); cla(state.handles.ax2); cla(state.handles.ax3);
        set([state.handles.ax1, state.handles.ax2, state.handles.ax3], 'Visible', 'on');

        switch source_idx
            case 1
                draw_single_dataset(state.pc, current_row, mode_idx, state.handles.ax1, state.handles.ax2, state.handles.ax3, 'PC');
            case 2
                draw_single_dataset(state.mtd, current_row, mode_idx, state.handles.ax1, state.handles.ax2, state.handles.ax3, 'Exported MTD');
            case 3
                draw_single_dataset(state.mtd_from_pc, current_row, mode_idx, state.handles.ax1, state.handles.ax2, state.handles.ax3, 'Recomputed MTD from PC');
            case 4
                draw_compare_dataset(state.mtd_from_pc, state.mtd, current_row, mode_idx, state.handles.ax1, state.handles.ax2, state.handles.ax3);
        end
    end
end

function draw_single_dataset(data, row_idx, mode_idx, ax1, ax2, ax3, label)
    if isempty(data)
        draw_empty(ax1, [label, ' not available']);
        draw_empty(ax2, '');
        draw_empty(ax3, '');
        return;
    end

    row_idx = min(max(1, row_idx), data.rows);
    main_matrix = get_mode_matrix(data, mode_idx);
    row_wave = get_mode_row(data, row_idx, mode_idx);

    axes(ax1);
    imagesc(main_matrix);
    axis tight;
    colorbar;
    hold on;
    plot([1, size(main_matrix, 2)], [row_idx, row_idx], 'w--', 'LineWidth', 1.2);
    hold off;
    title(sprintf('%s - %s', label, get_mode_name(mode_idx)));
    xlabel('Column');
    ylabel('Range Row');

    axes(ax2);
    plot(row_wave, 'LineWidth', 1.2);
    grid on;
    title(sprintf('%s - Row %d Waveform', label, row_idx));
    xlabel('Column Index');
    ylabel(get_mode_ylabel(mode_idx));

    axes(ax3);
    render_overview(ax3, data, label);
end

function draw_compare_dataset(data_a, data_b, row_idx, mode_idx, ax1, ax2, ax3)
    if isempty(data_a)
        draw_empty(ax1, 'Recomputed MTD from PC not available');
        draw_empty(ax2, '');
        draw_empty(ax3, '');
        return;
    end

    mat_a = get_mode_matrix(data_a, mode_idx);
    row_idx = min(max(1, row_idx), size(mat_a, 1));

    axes(ax1);
    imagesc(mat_a);
    axis tight;
    colorbar;
    hold on;
    plot([1, size(mat_a, 2)], [row_idx, row_idx], 'w--', 'LineWidth', 1.2);
    hold off;
    title({'Recomputed MTD', 'Source: recomputed from PC'});
    xlabel('Column');
    ylabel('Range Row');

    if isempty(data_b)
        draw_empty(ax2, 'Exported MTD not available');
        draw_empty(ax3, 'Difference unavailable');
        return;
    end

    mat_b = get_mode_matrix(data_b, mode_idx);
    common_rows = min(size(mat_a, 1), size(mat_b, 1));
    common_cols = min(size(mat_a, 2), size(mat_b, 2));
    row_idx = min(row_idx, common_rows);
    mat_a = mat_a(1:common_rows, 1:common_cols);
    mat_b = mat_b(1:common_rows, 1:common_cols);
    diff_mat = abs(mat_a - mat_b);

    axes(ax2);
    imagesc(mat_b);
    axis tight;
    colorbar;
    hold on;
    plot([1, size(mat_b, 2)], [row_idx, row_idx], 'w--', 'LineWidth', 1.2);
    hold off;
    title({'Exported MTD', 'Source: loaded from exported MTD'});
    xlabel('Column');
    ylabel('Range Row');

    axes(ax3);
    if mode_idx <= 3
        imagesc(diff_mat);
        axis tight;
        colorbar;
        hold on;
        plot([1, size(diff_mat, 2)], [row_idx, row_idx], 'w--', 'LineWidth', 1.2);
        hold off;
        title({'Difference Map |A-B|', 'Source: recomputed from PC vs loaded from exported MTD'});
        xlabel('Column');
        ylabel('Range Row');
    else
        row_a = get_mode_row(data_a, row_idx, mode_idx);
        row_b = get_mode_row(data_b, row_idx, mode_idx);
        n = min(numel(row_a), numel(row_b));
        plot(1:n, row_a(1:n), 'LineWidth', 1.2);
        hold on;
        plot(1:n, row_b(1:n), 'LineWidth', 1.2);
        plot(1:n, abs(row_a(1:n) - row_b(1:n)), '--', 'LineWidth', 1.0);
        hold off;
        grid on;
        legend({'Recomputed from PC', 'Loaded from exported MTD', 'Absolute Difference'}, 'Location', 'best');
        title(sprintf('Row %d Waveform Compare', row_idx));
        xlabel('Column Index');
        ylabel(get_mode_ylabel(mode_idx));
    end
end

function render_overview(ax, data, label)
    axes(ax);
    plot(data.raw, 'LineWidth', 1.0);
    grid on;
    title(sprintf('%s - Raw Float Stream', label));
    xlabel('Flat Index');
    ylabel('Value');
end

function draw_empty(ax, msg)
    axes(ax);
    cla(ax);
    axis(ax, 'off');
    if ~isempty(msg)
        text(0.1, 0.5, msg, 'Parent', ax, 'FontSize', 12);
    end
end

function matrix = get_mode_matrix(data, mode_idx)
    switch mode_idx
        case 1
            matrix = data.amplitude;
        case 2
            matrix = data.real_part;
        case 3
            matrix = data.imag_part;
        case 4
            matrix = data.amplitude;
        case 5
            matrix = data.real_part;
        case 6
            matrix = data.imag_part;
        otherwise
            matrix = data.amplitude;
    end
end

function row_wave = get_mode_row(data, row_idx, mode_idx)
    row_idx = min(max(1, row_idx), data.rows);
    switch mode_idx
        case {1, 4}
            row_wave = data.amplitude(row_idx, :);
        case {2, 5}
            row_wave = data.real_part(row_idx, :);
        case {3, 6}
            row_wave = data.imag_part(row_idx, :);
        otherwise
            row_wave = data.amplitude(row_idx, :);
    end
end

function name = get_mode_name(mode_idx)
    names = {'Magnitude Image', 'I Image', 'Q Image', 'Magnitude Waveform', 'I Waveform', 'Q Waveform'};
    name = names{min(max(1, mode_idx), numel(names))};
end

function name = get_mode_ylabel(mode_idx)
    switch mode_idx
        case {1, 4}
            name = 'Magnitude';
        case {2, 5}
            name = 'I';
        case {3, 6}
            name = 'Q';
        otherwise
            name = 'Value';
    end
end

function [pc_file, mtd_file, prefix] = resolve_pair_files(input_path)
    if exist(input_path, 'dir') == 7
        error('Input must be a file path or export prefix, not a folder.');
    end

    [folder_path, base_name, ext] = fileparts(input_path);
    if isempty(folder_path)
        folder_path = pwd;
    end

    prefix = fullfile(folder_path, base_name);
    prefix = regexprep(prefix, '_(pc|mtd)$', '');
    if strcmpi(ext, '.dat')
        prefix = regexprep(fullfile(folder_path, base_name), '_(pc|mtd)$', '');
    end

    pc_list = dir([prefix, '_pc*.dat']);
    mtd_list = dir([prefix, '_mtd*.dat']);

    if isempty(pc_list)
        pc_file = '';
    else
        pc_file = fullfile(pc_list(1).folder, pc_list(1).name);
    end

    if isempty(mtd_list)
        mtd_file = '';
    else
        mtd_file = fullfile(mtd_list(1).folder, mtd_list(1).name);
    end
end

function export_data = load_export_dat(file_path)
    [~, ~, ext] = fileparts(file_path);
    if ~strcmpi(ext, '.dat')
        error('Only .dat files are supported: %s', file_path);
    end
    export_data.file_path = file_path;

    fid = fopen(file_path, 'rb');
    if fid == -1
        error('Failed to open file: %s', file_path);
    end

    magic = char(fread(fid, 4, 'uint8=>char')');
    version = fread(fid, 1, 'uint8=>double');
    data_type = fread(fid, 1, 'uint8=>double');
    rows = fread(fid, 1, 'uint16=>double');
    cols = fread(fid, 1, 'uint16=>double');
    channels = fread(fid, 1, 'uint8=>double');
    fread(fid, 1, 'uint8=>double');
    data_count = fread(fid, 1, 'uint32=>double');

    if numel(magic) ~= 4 || ~strcmp(magic, 'SPDT')
        fclose(fid);
        error('Invalid export file header: %s', file_path);
    end
    if version ~= 1
        fclose(fid);
        error('Unsupported export file version: %d, file=%s', version, file_path);
    end

    if data_type == 1
        export_data.kind = 'PC';
    elseif data_type == 2
        export_data.kind = 'MTD';
    else
        fclose(fid);
        error('Unknown export data type: %d, file=%s', data_type, file_path);
    end

    export_data.rows = rows;
    export_data.cols = cols;
    export_data.channels = channels;
    export_data.version = version;
    export_data.data_type = data_type;

    raw = fread(fid, inf, 'float32=>single');
    fclose(fid);

    expected_count = export_data.rows * export_data.cols * export_data.channels;
    if data_count ~= expected_count
        error('Header dataCount mismatch. expected=%d actual=%d file=%s', expected_count, data_count, file_path);
    end
    if numel(raw) ~= expected_count
        error('Data length mismatch. expected=%d actual=%d file=%s', expected_count, numel(raw), file_path);
    end

    cube = reshape(raw, [export_data.channels, export_data.cols, export_data.rows]);
    cube = permute(cube, [3, 2, 1]);
    export_data.raw = raw;
    export_data.cube = cube;

    if export_data.channels >= 2
        export_data.real_part = squeeze(cube(:, :, 1));
        export_data.imag_part = squeeze(cube(:, :, 2));
        export_data.complex_data = complex(export_data.real_part, export_data.imag_part);
        export_data.amplitude = abs(export_data.complex_data);
    else
        export_data.real_part = squeeze(cube(:, :, 1));
        export_data.imag_part = zeros(size(export_data.real_part), 'like', export_data.real_part);
        export_data.complex_data = complex(export_data.real_part, export_data.imag_part);
        export_data.amplitude = export_data.real_part;
    end
end

function mtd_data = calc_mtd_from_pc(pc_data)
    if pc_data.channels < 2 || isempty(pc_data.complex_data)
        error('PC data must contain IQ channels to calculate MTD.');
    end

    mtd_complex = fftshift(fft(pc_data.complex_data, [], 2), 2);

    mtd_data = struct();
    mtd_data.kind = 'MTD_FROM_PC';
    mtd_data.rows = size(mtd_complex, 1);
    mtd_data.cols = size(mtd_complex, 2);
    mtd_data.channels = 2;
    mtd_data.file_path = [pc_data.file_path, ' -> MATLAB FFT'];
    mtd_data.complex_data = mtd_complex;
    mtd_data.real_part = real(mtd_complex);
    mtd_data.imag_part = imag(mtd_complex);
    mtd_data.amplitude = abs(mtd_complex);
    mtd_data.raw = [mtd_data.real_part(:); mtd_data.imag_part(:)];
end
