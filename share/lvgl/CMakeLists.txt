include_directories(
    .
    ${PROJECT_PATH}/common/
    ${PROJECT_PATH}/
    ${PROJECT_PATH}/share/
    ${PROJECT_PATH}/share/md5/
    ${PROJECT_PATH}/share/freetype-2.9.1/include/
)

set(corepath ${PROJECT_PATH}/share/lvgl/src/core)

set(corelib 
    ${corepath}/lv_obj.c
    ${corepath}/lv_refr.c
    ${corepath}/lv_theme.c
    ${corepath}/lv_obj_tree.c 
    ${corepath}/lv_obj_style_gen.c 
    ${corepath}/lv_obj_style.c
    ${corepath}/lv_obj_scroll.c 
    ${corepath}/lv_obj_pos.c 
    ${corepath}/lv_obj_draw.c 
    ${corepath}/lv_obj_class.c
    ${corepath}/lv_indev.c
    ${corepath}/lv_indev_scroll.c 
    ${corepath}/lv_group.c 
    ${corepath}/lv_event.c 
    ${corepath}/lv_disp.c 
)


set(drawpath ${PROJECT_PATH}/share/lvgl/src/draw)
set(drawlib 
    ${drawpath}/lv_draw_arc.c 
    ${drawpath}/lv_draw_img.c 
    ${drawpath}/lv_draw_label.c 
    ${drawpath}/lv_draw_layer.c 
    ${drawpath}/lv_draw_line.c 
    ${drawpath}/lv_draw_mask.c 
    ${drawpath}/lv_draw_rect.c 
    ${drawpath}/lv_draw_transform.c 
    ${drawpath}/lv_draw_triangle.c 
    ${drawpath}/lv_draw.c 
    ${drawpath}/lv_img_buf.c 
    ${drawpath}/lv_img_cache.c 
    ${drawpath}/lv_img_decoder.c
)
set(drawswpath ${PROJECT_PATH}/share/lvgl/src/draw/sw)
set(drawswlib   
   ${drawswpath}/lv_draw_sw_arc.c
   ${drawswpath}/lv_draw_sw_blend.c
   ${drawswpath}/lv_draw_sw_dither.c 
   ${drawswpath}/lv_draw_sw_gradient.c
   ${drawswpath}/lv_draw_sw_img.c 
   ${drawswpath}/lv_draw_sw_layer.c
   ${drawswpath}/lv_draw_sw_letter.c 
   ${drawswpath}/lv_draw_sw_line.c
   ${drawswpath}/lv_draw_sw_transform.c 
   ${drawswpath}/lv_draw_sw.c
   ${drawswpath}/lv_draw_sw_rect.c
   ${drawswpath}/lv_draw_sw_polygon.c
)

set(miscpath ${PROJECT_PATH}/share/lvgl/src/misc)
set(misclib  
   ${miscpath}/lv_anim_timeline.c
   ${miscpath}/lv_anim.c 
   ${miscpath}/lv_area.c 
   ${miscpath}/lv_async.c 
   ${miscpath}/lv_bidi.c 
   ${miscpath}/lv_color.c 
   ${miscpath}/lv_fs.c
   ${miscpath}/lv_gc.c 
   ${miscpath}/lv_ll.c 
   ${miscpath}/lv_log.c 
   ${miscpath}/lv_lru.c 
   ${miscpath}/lv_math.c
   ${miscpath}/lv_mem.c 
   ${miscpath}/lv_printf.c 
   ${miscpath}/lv_style_gen.c 
   ${miscpath}/lv_style.c 
   ${miscpath}/lv_templ.c 
   ${miscpath}/lv_timer.c 
   ${miscpath}/lv_tlsf.c 
   ${miscpath}/lv_txt_ap.c 
   ${miscpath}/lv_txt.c
   ${miscpath}/lv_utils.c 
)

set(halpath ${PROJECT_PATH}/share/lvgl/src/hal)
set(hallib   
    ${halpath}/lv_hal_disp.c
    ${halpath}/lv_hal_indev.c 
    ${halpath}/lv_hal_tick.c
)


set(fontpath ${PROJECT_PATH}/share/lvgl/src/font)
set(fontlib 
   ${fontpath}/lv_font.c
   ${fontpath}/lv_font_fmt_txt.c  
   ${fontpath}/lv_font_montserrat_14.c  
   ${fontpath}/lv_font_montserrat_12.c  
   ${fontpath}/lv_font_montserrat_16.c 
   ${fontpath}/lv_font_montserrat_22.c 
   ${fontpath}/lv_font_montserrat_32.c   
)

set(extrapath ${PROJECT_PATH}/share/lvgl/src/extra)
set(extralib 
    ${extrapath}/lv_extra.c
    ${extrapath}/widgets/win/lv_win.c
    ${extrapath}/widgets/tileview/lv_tileview.c
    ${extrapath}/widgets/tabview/lv_tabview.c
    ${extrapath}/widgets/spinner/lv_spinner.c
    ${extrapath}/widgets/spinbox/lv_spinbox.c
    ${extrapath}/widgets/span/lv_span.c 
    ${extrapath}/widgets/msgbox/lv_msgbox.c 
    ${extrapath}/widgets/meter/lv_meter.c 
    ${extrapath}/widgets/menu/lv_menu.c 
    ${extrapath}/widgets/list/lv_list.c 
    ${extrapath}/widgets/led/lv_led.c
    ${extrapath}/widgets/keyboard/lv_keyboard.c
    ${extrapath}/widgets/imgbtn/lv_imgbtn.c 
    ${extrapath}/widgets/colorwheel/lv_colorwheel.c 
    ${extrapath}/widgets/chart/lv_chart.c 
    ${extrapath}/widgets/calendar/lv_calendar.c
    ${extrapath}/widgets/calendar/lv_calendar_header_arrow.c
    ${extrapath}/widgets/calendar/lv_calendar_header_dropdown.c
    ${extrapath}/widgets/animimg/lv_animimg.c
    ${extrapath}/themes/default/lv_theme_default.c
    ${extrapath}/layouts/flex/lv_flex.c
    ${extrapath}/layouts/grid/lv_grid.c
    ${extrapath}/libs/freetype/lv_freetype.c
    ${extrapath}/libs/png/lv_png.c
    ${extrapath}/libs/png/lodepng.c
    ${extrapath}/libs/jpg/lv_jpg.c
    ${extrapath}/libs/fsdrv/lv_fs_stdio.c
    ${extrapath}/libs/mp4/lv_mp4.c
    ${extrapath}/libs/mp4/lv_mp4.c
    ${extrapath}/libs/argb32/lv_argb32_data.c
    ${extrapath}/libs/sjpg/lv_sjpg.c
    ${extrapath}/libs/sjpg/tjpgd.c
    ${extrapath}/libs/fsdrv/lv_fs_posix.c
    ${extrapath}/libs/qrcode/lv_qrcode.c
    ${extrapath}/libs/qrcode/qrcodegen.c
)

set(widgetspath ${PROJECT_PATH}/share/lvgl/src/widgets)
set(widgetslib 
    ${widgetspath}/lv_arc.c 
    ${widgetspath}/lv_bar.c 
    ${widgetspath}/lv_btnmatrix.c 
    ${widgetspath}/lv_canvas.c 
    ${widgetspath}/lv_checkbox.c 
    ${widgetspath}/lv_dropdown.c 
    ${widgetspath}/lv_img.c 
    ${widgetspath}/lv_label.c 
    ${widgetspath}/lv_line.c
    ${widgetspath}/lv_objx_templ.c 
    ${widgetspath}/lv_roller.c 
    ${widgetspath}/lv_slider.c 
    ${widgetspath}/lv_switch.c 
    ${widgetspath}/lv_table.c 
    ${widgetspath}/lv_textarea.c
    ${widgetspath}/lv_btn.c 
)

set(benchmarkpath ${PROJECT_PATH}/share/lvgl/demos/benchmark)
set(benchmarklib 
    ${benchmarkpath}/lv_demo_benchmark.c
    ${benchmarkpath}/assets/img_benchmark_cogwheel_alpha16.c
    ${benchmarkpath}/assets/img_benchmark_cogwheel_argb.c
    ${benchmarkpath}/assets/img_benchmark_cogwheel_chroma_keyed.c
    ${benchmarkpath}/assets/img_benchmark_cogwheel_indexed16.c
    ${benchmarkpath}/assets/img_benchmark_cogwheel_rgb.c
    ${benchmarkpath}/assets/img_benchmark_cogwheel_rgb565a8.c
    ${benchmarkpath}/assets/lv_font_bechmark_montserrat_12_compr_az.c.c
    ${benchmarkpath}/assets/lv_font_bechmark_montserrat_16_compr_az.c.c
    ${benchmarkpath}/assets/lv_font_bechmark_montserrat_28_compr_az.c.c
)

set(musicpath ${PROJECT_PATH}/share/lvgl/demos/music)
set(musiclib 
    ${musicpath}/lv_demo_music.c
    ${musicpath}/lv_demo_music_main.c
    ${musicpath}/lv_demo_music_list.c
    ${musicpath}/assets/img_lv_demo_music_logo.c
    ${musicpath}/assets/img_lv_demo_music_btn_list_pause.c
    ${musicpath}/assets/img_lv_demo_music_btn_list_play.c
    ${musicpath}/assets/img_lv_demo_music_btn_loop.c
    ${musicpath}/assets/img_lv_demo_music_icon_1.c
    ${musicpath}/assets/img_lv_demo_music_icon_2.c
    ${musicpath}/assets/img_lv_demo_music_icon_3.c
    ${musicpath}/assets/img_lv_demo_music_icon_4.c
    ${musicpath}/assets/img_lv_demo_music_slider_knob.c
    ${musicpath}/assets/img_lv_demo_music_corner_left.c
    ${musicpath}/assets/img_lv_demo_music_cover_1.c
    ${musicpath}/assets/img_lv_demo_music_cover_2.c
    ${musicpath}/assets/img_lv_demo_music_cover_3.c
    ${musicpath}/assets/img_lv_demo_music_wave_bottom.c
    ${musicpath}/assets/img_lv_demo_music_list_border.c
    ${musicpath}/assets/img_lv_demo_music_btn_next.c
    ${musicpath}/assets/img_lv_demo_music_btn_play.c
    ${musicpath}/assets/img_lv_demo_music_btn_pause.c
    ${musicpath}/assets/img_lv_demo_music_btn_prev.c
    ${musicpath}/assets/img_lv_demo_music_btn_rnd.c
    ${musicpath}/assets/img_lv_demo_music_corner_right.c
    ${musicpath}/assets/img_lv_demo_music_wave_top.c
)
set(demowidgetspath ${PROJECT_PATH}/share/lvgl/demos/widgets)
set(demowidgetslib 
    ${demowidgetspath}/lv_demo_widgets.c
    ${demowidgetspath}/assets/img_clothes.c
    ${demowidgetspath}/assets/img_demo_widgets_avatar.c    
    ${demowidgetspath}/assets/img_lvgl_logo.c

)
set(devicepath ${PROJECT_PATH}/share/lvgl/device)
set(devicepathlib 
    ${devicepath}/lv_disp_dev.c
    ${devicepath}/lv_input_dev.c
    ${devicepath}/lv_firmware_resource.c
    #${devicepath}/lv_firmware_arc.c
    #${devicepath}/lv_firmware_bar.c    
    #${devicepath}/lv_firmware_button.c  
    #${devicepath}/lv_firmware_clock.c
    #${devicepath}/lv_firmware_icon.c
    #${devicepath}/lv_firmware_label.c
    #${devicepath}/lv_firmware_line.c
    #${devicepath}/lv_firmware_meter.c
    #${devicepath}/lv_firmware_slider.c
    #${devicepath}/lv_firmware.c

)


add_library(lvgl OBJECT
    ${corelib}
    ${drawlib}
    ${drawswlib}
    ${misclib}
    ${hallib}
    ${fontlib}
    ${extralib}
    ${widgetslib}
    ${devicepathlib}
    #${benchmarklib}
    #${musiclib}
    #${demowidgetslib}
)
