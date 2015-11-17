
#
# This file is the default set of rules to compile a Pebble project.
#
# Feel free to customize this to your needs.
#

import os.path

top = '.'
out = 'build'

def options(ctx):
    ctx.load('pebble_sdk')

def configure(ctx):
    ctx.load('pebble_sdk')

def build(ctx):
    ctx.load('pebble_sdk')

	# Compile the dictionaries into resources
#ctx(rule='python ../dict2bin.py ${SRC}', source='../locale_english.json', target='../locale_english.bin')
#ctx(rule='python ../dict2bin.py ${SRC}', source='../locale_german.json', target='../locale_german.bin')
#ctx(rule='python ../dict2bin.py ${SRC}', source='../locale_french.json', target='../locale_french.bin')
#ctx(rule='python ../dict2bin.py ${SRC}', source='../locale_spanish.json', target='../locale_spanish.bin')
#additional_resources = ['../resources/locale_spanish.bin','../resources/locale_german.bin','../resources/locale_french.bin','../resources/locale_spanish.bin']
#ctx(rule='mv ../*.bin ../resources',
#        source = ['../locale_spanish.bin','../locale_german.bin','../locale_french.bin','../locale_spanish.bin'],
#       target = additional_resources)
    build_worker = os.path.exists('worker_src')
    binaries = []

    for p in ctx.env.TARGET_PLATFORMS:
        ctx.set_env(ctx.all_envs[p])
        ctx.set_group(ctx.env.PLATFORM_NAME)
        app_elf='{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        source_files = ctx.path.ant_glob('src/**/*.c')
        #source_files.append(additional_resources)
        #print source_files
        ctx.pbl_program(source=source_files,
        target=app_elf)

        if build_worker:
            worker_elf='{}/pebble-worker.elf'.format(ctx.env.BUILD_DIR)
            binaries.append({'platform': p, 'app_elf': app_elf, 'worker_elf': worker_elf})
            ctx.pbl_worker(source=ctx.path.ant_glob('worker_src/**/*.c'),
            target=worker_elf)
        else:
            binaries.append({'platform': p, 'app_elf': app_elf})

    ctx.pbl_bundle(binaries=binaries, js=ctx.path.ant_glob('src/js/**/*.js'))
