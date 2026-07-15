%% 读取float32数据并整理为256规格的二维矩阵 + 绘制三维图
% 解决文件读取问题 + 核心：一维float数据→二维矩阵→二维热力图+三维图
clc; clear all; close all;

% ====================== 核心配置（按需修改） ======================
matrix_dim_mode = 'cols';   % 矩阵维度模式：'rows'=256行 / 'cols'=256列
fixed_dim_size = 256;       % 固定维度（无需修改，保持256）
matrix_folder = '.\matrix'; % 【按需修改】matrix文件夹路径（相对/绝对均可）
file_name = 'mtd_echo_matrix_3.dat'; % 目标.dat文件名
plot_3d_type = 'surf';      % 三维图类型：'surf'=曲面图（带填充） / 'mesh'=网格图（仅线条）

% ====================== 1. 拼接绝对路径 + 校验文件 ======================
file_path = fullfile(matrix_folder, file_name);

% 检查matrix文件夹是否存在
if ~exist(matrix_folder, 'dir')
    error('matrix文件夹不存在！路径：%s', matrix_folder);
end

% 检查目标文件是否存在
if ~exist(file_path, 'file')
    error('目标文件不存在！路径：%s', file_path);
end

% ====================== 2. 读取float32格式的一维数据 ======================
fid = fopen(file_path, 'rb');
if fid == -1
    error('文件无法打开（权限/损坏）：%s', file_path);
end
data_float32 = fread(fid, inf, 'float32'); % 读取所有float32数据
fclose(fid);

% 校验读取的一维数据长度
data_len = length(data_float32);
if data_len == 0
    error('文件中无有效float32数据：%s', file_path);
end
disp('=============================================');
disp(['? 成功读取文件：', file_path]);
disp(['一维float32数据长度：', num2str(data_len)]);

% ====================== 3. 核心：整理为二维矩阵 ======================
try
    if strcmp(matrix_dim_mode, 'rows')
        % 模式1：256行，列数 = 总长度 / 256
        if mod(data_len, fixed_dim_size) ~= 0
            error('数据长度(%d)无法被256整除，无法整理为256行的矩阵', data_len);
        end
        matrix_2d = reshape(data_float32, fixed_dim_size, []); % 256行 × N列
        matrix_shape = size(matrix_2d);
        disp(['? 整理为二维矩阵（256行）：', num2str(matrix_shape(1)), '行 × ', num2str(matrix_shape(2)), '列']);

    elseif strcmp(matrix_dim_mode, 'cols')
        % 模式2：256列，行数 = 总长度 / 256
        if mod(data_len, fixed_dim_size) ~= 0
            error('数据长度(%d)无法被256整除，无法整理为256列的矩阵', data_len);
        end
        matrix_2d = reshape(data_float32, [], fixed_dim_size); % N行 × 256列
        matrix_shape = size(matrix_2d);
        disp(['? 整理为二维矩阵（256列）：', num2str(matrix_shape(1)), '行 × ', num2str(matrix_shape(2)), '列']);

    else
        error('维度模式错误！仅支持''rows''或''cols''');
    end
catch ME
    error('整理二维矩阵失败：%s', ME.message);
end

% ====================== 4. 验证二维矩阵（可选，确认数据正确性） ======================
disp('=============================================');
disp('二维矩阵验证信息：');
disp(['  矩阵数据类型：', class(matrix_2d)]); % 应为single（float32）
disp(['  矩阵数值范围：[', num2str(min(matrix_2d(:))), ', ', num2str(max(matrix_2d(:))), ']']);
disp(['  矩阵总元素数：', num2str(numel(matrix_2d)), '（与一维数据长度一致：', num2str(data_len), '）']);
disp('=============================================');

% ====================== 5. 绘制二维矩阵热力图（直观查看） ======================
figure('Name', '256规格二维矩阵热力图');
imagesc(matrix_2d);
colormap('jet'); % 配色方案
colorbar; % 颜色刻度条
axis image; % 保持行列比例
title(['二维矩阵 ', num2str(matrix_shape(1)), '×', num2str(matrix_shape(2))]);
xlabel('列索引');
ylabel('行索引');
grid on;

% ====================== 6. 新增：绘制三维图（曲面/网格） ======================
% 生成X/Y轴坐标网格（匹配矩阵行列）
[X, Y] = meshgrid(1:matrix_shape(2), 1:matrix_shape(1));

% 创建独立的三维图窗口
figure('Name', '256规格矩阵三维图', 'Position', [200, 200, 1200, 800]);
hold on; grid on; box on;

% 根据配置绘制曲面图/网格图
if strcmp(plot_3d_type, 'surf')
    % 曲面图：带颜色填充，更直观展示数值分布
    surf(X, Y, matrix_2d, 'EdgeColor', 'none', 'FaceAlpha', 0.8); % 关闭边缘线，半透明
elseif strcmp(plot_3d_type, 'mesh')
    % 网格图：仅显示网格线，轻量化，突出轮廓
    mesh(X, Y, matrix_2d, 'LineWidth', 0.5);
else
    warning('三维图类型配置错误，默认绘制曲面图');
    surf(X, Y, matrix_2d, 'EdgeColor', 'none', 'FaceAlpha', 0.8);
end

% 美化三维图
colormap('jet'); % 与二维图配色一致，便于对比
colorbar; % 颜色刻度条（Z轴数值对应颜色）
xlabel('列索引 (Column Index)', 'FontSize', 10);
ylabel('行索引 (Row Index)', 'FontSize', 10);
zlabel('幅值 (Amplitude)', 'FontSize', 10);
title(['三维', upper(plot_3d_type), '图 - ', num2str(matrix_shape(1)), '×', num2str(matrix_shape(2))], 'FontSize', 12);
set(gca, 'FontSize', 9);

% 启用三维交互：可手动旋转/缩放
view(3); % 初始三维视角
rotate3d on; % 开启鼠标旋转功能
camlight; % 添加光源，增强立体感
lighting gouraud; % 平滑光照，提升视觉效果

disp('? 三维图绘制完成！可手动拖动鼠标旋转视角，滚轮缩放');