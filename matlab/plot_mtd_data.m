%% Read and plot MTD echo data
% 功能：读取二进制MTD回波数据（float32格式）并可视化，支持文件选择和结果保存
% 用法：
% 1. 命令行调用：plot_mtd_data('mtd_echo_data_10.dat')
% 2. 直接运行脚本：弹出文件选择对话框选择.dat文件
% 作者：优化版
% 版本：1.0

function plot_mtd_data(varargin)
    % ====================== 1. 处理输入参数/文件选择 ======================
    if nargin == 0
        % 弹出文件选择对话框，增强过滤条件
        [filename, pathname] = uigetfile( ...
            {'*.dat', 'MTD Echo Data Files (*.dat)'; '*.*', 'All Files (*.*)'}, ...
            'Select MTD Echo Data File', ...
            pwd); % 默认路径为当前工作目录
        
        % 用户取消选择时优雅退出
        if isequal(filename, 0)
            disp('INFO: User canceled file selection.');
            return;
        end
        % 安全拼接文件路径（避免路径分隔符问题）
        full_filename = fullfile(pathname, filename);
    else
        % 验证输入参数为字符串
        if ~ischar(varargin{1})
            error('Input argument must be a valid file path string!');
        end
        full_filename = varargin{1};
    end

    % ====================== 2. 验证文件有效性 ======================
    % 检查文件是否存在
    if ~exist(full_filename, 'file')
        error('File not found: %s', full_filename);
    end
    % 检查文件是否为可读的普通文件（排除文件夹/链接）
    file_info = dir(full_filename);
    if file_info.isdir
        error('Input path is a directory, not a file: %s', full_filename);
    end

    % ====================== 3. 读取二进制数据 ======================
    fid = fopen(full_filename, 'rb');
    % 增强文件打开失败的错误提示
    if fid == -1
        error('Failed to open file for reading (permission/format issue): %s', full_filename);
    end
    
    % 读取数据并捕获读取错误
    try
        data = fread(fid, inf, 'float32'); % 使用inf读取全部数据，更规范
    catch ME
        fclose(fid); % 确保文件句柄关闭
        error('Failed to read data from file: %s\nError details: %s', full_filename, ME.message);
    end
    fclose(fid); % 确保文件句柄关闭

    % ====================== 4. 数据有效性检查 ======================
    if isempty(data)
        warning('File contains no valid float32 data: %s', full_filename);
        return;
    end

    % ====================== 5. 显示数据基本信息 ======================
    disp('=============================================');
    disp(['File path: ', full_filename]);
    disp(['Data length: ', num2str(length(data))]);
    disp(['Data range: [', num2str(min(data)), ', ', num2str(max(data)), ']']);
    disp(['Mean value: ', num2str(mean(data))]); % 新增均值，增强信息
    disp('=============================================');

    % ====================== 6. 绘制波形图 ======================
    % 创建独立的图形窗口，避免覆盖现有图窗
    fig = figure('Name', ['MTD Echo Data - ', file_info.name], ...
        'Position', [100, 100, 1200, 600]); % 固定窗口大小，提升体验
    
    % 绘制波形，添加线条样式增强可读性
    plot(data, 'Color', [0.2, 0.4, 0.8], 'LineWidth', 1);
    grid on;
    grid minor; % 显示次要网格线
    title(['MTD Echo Data Waveform - ', file_info.name], 'FontSize', 12);
    xlabel('Sample Index', 'FontSize', 10);
    ylabel('Amplitude', 'FontSize', 10);
    set(gca, 'FontSize', 9); % 调整坐标轴字体大小

    % ====================== 7. 缩放功能（长数据） ======================
    if length(data) > 1000
        zoom('on'); % 规范的zoom调用方式
        pan('on'); % 新增平移功能，无需Shift键
        disp('Tip: Use mouse wheel to zoom, drag mouse to pan view (no Shift needed)');
    end

    % ====================== 8. 保存图片（增强路径处理） ======================
    % 保存到原文件同目录，避免路径混乱
    save_path = fullfile(file_info.folder, [file_info.name(1:end-4), '.png']);
    saveas(fig, save_path);
    disp(['Image saved successfully: ', save_path]);
end