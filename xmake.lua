add_rules("mode.debug", "mode.release")
add_cxflags("/WX-", {force = true}) -- 关闭警告视为错误
add_cxflags("/wd4090", {force = true}) -- 忽略C4090警告

set_languages("c11")

-- 指定头文件目录
add_includedirs("include")

-- 定义目标可执行文件
target("hercode_compiler")
    set_kind("binary")
    add_files("src/*.c")
    add_includedirs("include")

    -- Windows下特殊设置
    if is_plat("windows") then
        add_cxflags("/W4", "/WX", "/utf-8")
        add_defines("_CRT_SECURE_NO_WARNINGS")
    else
        add_cxflags("-Wall", "-Werror", "-Wstrict-prototypes", "-Wmissing-prototypes", "-O2", "-Os")
    end

    -- 构建后自动复制到test目录
    after_build(function (target)
        os.mkdir("test")
        os.cp(target:targetfile(), path.join("test", path.filename(target:targetfile())))
    end)
