{
	"patcher" : 	{
		"fileversion" : 1,
		"appversion" : 		{
			"major" : 6,
			"minor" : 0,
			"revision" : 8
		}
,
		"rect" : [ 121.0, 44.0, 980.0, 660.0 ],
		"bglocked" : 0,
		"openinpresentation" : 0,
		"default_fontsize" : 10.0,
		"default_fontface" : 0,
		"default_fontname" : "Verdana",
		"gridonopen" : 0,
		"gridsize" : [ 5.0, 5.0 ],
		"gridsnaponopen" : 0,
		"statusbarvisible" : 2,
		"toolbarvisible" : 1,
		"boxanimatetime" : 200,
		"imprint" : 0,
		"enablehscroll" : 1,
		"enablevscroll" : 1,
		"devicewidth" : 0.0,
		"description" : "",
		"digest" : "",
		"tags" : "",
		"boxes" : [ 			{
				"box" : 				{
					"id" : "obj-17",
					"maxclass" : "bpatcher",
					"name" : "jmod.libmapper.maxpat",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 355.0, 93.0, 600.0, 70.0 ],
					"presentation_rect" : [ 0.0, 0.0, 600.0, 70.0 ]
				}

			}
, 			{
				"box" : 				{
					"args" : [ "/output~" ],
					"id" : "obj-2",
					"lockeddragscroll" : 1,
					"maxclass" : "bpatcher",
					"name" : "jmod.output~.maxpat",
					"numinlets" : 3,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 18.0, 490.0, 300.0, 140.0 ],
					"presentation_rect" : [ 22.0, 472.0, 300.0, 140.0 ],
					"varname" : "/input~[1]"
				}

			}
, 			{
				"box" : 				{
					"args" : [ "/cueManager" ],
					"id" : "obj-1",
					"maxclass" : "bpatcher",
					"name" : "jmod.cueManager.maxpat",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 356.0, 364.0, 300.0, 70.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 50.0, 115.0, 300.0, 70.0 ]
				}

			}
, 			{
				"box" : 				{
					"args" : [ "/bcf2000" ],
					"id" : "obj-8",
					"lockeddragscroll" : 1,
					"maxclass" : "bpatcher",
					"name" : "jmod.bcf2000.maxpat",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 604.0, 257.0, 300.0, 70.0 ],
					"presentation_rect" : [ 15.0, 15.0, 300.0, 70.0 ],
					"varname" : "jmod.bcf2000"
				}

			}
, 			{
				"box" : 				{
					"args" : [ "@name", "jmod.libmapper", "@description", "A bridge between Jamoma and the libmapper Mapping Network." ],
					"bgmode" : 1,
					"id" : "obj-31",
					"maxclass" : "bpatcher",
					"name" : "jcom.maxhelpui.maxpat",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 0.0, 0.0, 980.0, 70.0 ],
					"prototypename" : "bphelp",
					"varname" : "maxhelpui"
				}

			}
, 			{
				"box" : 				{
					"args" : [ "/input~" ],
					"id" : "obj-5",
					"lockeddragscroll" : 1,
					"maxclass" : "bpatcher",
					"name" : "jmod.input~.maxpat",
					"numinlets" : 1,
					"numoutlets" : 3,
					"outlettype" : [ "", "signal", "signal" ],
					"patching_rect" : [ 18.0, 253.0, 300.0, 140.0 ],
					"presentation_rect" : [ 0.0, 0.0, 300.0, 140.0 ],
					"varname" : "/input~"
				}

			}
, 			{
				"box" : 				{
					"args" : [ "/filter~" ],
					"id" : "obj-6",
					"lockeddragscroll" : 1,
					"maxclass" : "bpatcher",
					"name" : "jmod.filter~.maxpat",
					"numinlets" : 3,
					"numoutlets" : 3,
					"outlettype" : [ "", "signal", "signal" ],
					"patching_rect" : [ 18.0, 406.0, 300.0, 70.0 ],
					"presentation_rect" : [ 0.0, 0.0, 300.0, 70.0 ],
					"varname" : "/filter~"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Verdana",
					"fontsize" : 10.0,
					"frgb" : 0.0,
					"id" : "obj-22",
					"linecount" : 2,
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 17.0, 159.0, 285.0, 31.0 ],
					"text" : "Choose the source and destinations adresses, fine-tune the settings, and you're done."
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Verdana Bold",
					"fontsize" : 10.0,
					"frgb" : 0.0,
					"id" : "obj-27",
					"linecount" : 3,
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 15.0, 93.0, 309.0, 43.0 ],
					"text" : "This module allows one to map data from a module's output (return or parameter) to another module's parameter (or message)"
				}

			}
, 			{
				"box" : 				{
					"args" : [ "/mouse" ],
					"id" : "obj-28",
					"lockeddragscroll" : 1,
					"maxclass" : "bpatcher",
					"name" : "jmod.mouse.maxpat",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 355.0, 254.0, 150.0, 70.0 ],
					"presentation_rect" : [ 0.0, 0.0, 150.0, 70.0 ],
					"varname" : "/mouse/1"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "obj-6", 2 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-5", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-6", 1 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-5", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-2", 2 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-6", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-2", 1 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-6", 1 ]
				}

			}
 ],
		"dependency_cache" : [ 			{
				"name" : "jmod.mouse.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/modules/control/mouse",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/modules/control/mouse",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jalg.mouse.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/modules/control/mouse",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/modules/control/mouse",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jmod.filter~.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/modules/audio/filter~",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/modules/audio/filter~",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jalg.filter~.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/modules/audio/filter~",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/modules/audio/filter~",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jcom.meter_receive.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/library/components/meter_receive",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/library/components/meter_receive",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jmod.input~.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/modules/audio/input~",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/modules/audio/input~",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jcom.audioOnOff.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/library/components/audioOnOff",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/library/components/audioOnOff",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jcom.maxhelpui.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/library/components/maxhelpui",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/library/components/maxhelpui",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jcom.maxhelpuiButton.png",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/library/components/maxhelpui",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/library/components/maxhelpui",
				"type" : "PNG ",
				"implicit" : 1
			}
, 			{
				"name" : "jcom.maxhelpuiResize.js",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/library/components/maxhelpui",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/library/components/maxhelpui",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "jcom.jamomaPath.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/library/components/jamomaPath",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/library/components/jamomaPath",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jcom.thru.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/library/components/thru",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/library/components/thru",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jmod.bcf2000.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/modules/control/bcf2000",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/modules/control/bcf2000",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jalg.bcf2000.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/modules/control/bcf2000",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/modules/control/bcf2000",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "BCFFader.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/modules/control/bcf2000",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/modules/control/bcf2000",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "Push_encoder_turn.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/modules/control/bcf2000",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/modules/control/bcf2000",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "Push_encoder_press.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/modules/control/bcf2000",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/modules/control/bcf2000",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "BCFKey.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/modules/control/bcf2000",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/modules/control/bcf2000",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jmod.cueManager.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/modules/control/cueManager",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/modules/control/cueManager",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jcom.initialized.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/library/components/initialized",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/library/components/initialized",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jmod.output~.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/modules/audio/output~",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/modules/audio/output~",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jcom.js_systeminfo.js",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/library/javascript",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/library/javascript",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "jmod.libmapper.maxpat",
				"bootpath" : "/Users/jocal/Documents/Mappers/mapper-max-pd/bridges/jamoma/mapper",
				"patcherrelativepath" : "",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jalg.libmapper.maxpat",
				"bootpath" : "/Users/jocal/Documents/Mappers/mapper-max-pd/bridges/jamoma/mapper",
				"patcherrelativepath" : "",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jcom.modulesDumper.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/library/components/modulesDumper",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/library/components/modulesDumper",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jcom.getModuleNames.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/library/components/getModuleNames",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/library/components/getModuleNames",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jcom.getReturnNames.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/library/components/getReturnNames",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/library/components/getReturnNames",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jcom.getAllAttributes.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/library/components/getAllAttributes",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/library/components/getAllAttributes",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jcom.getParameterNames.maxpat",
				"bootpath" : "/Applications/Max6/Cycling '74/Jamoma/library/components/getParameterNames",
				"patcherrelativepath" : "../../../../../../../../Applications/Max6/Cycling '74/Jamoma/library/components/getParameterNames",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "dot.prependaddr.maxpat",
				"bootpath" : "/Users/jocal/Documents/DOT/branches/v2_max5/toolbox/osc/dot.prependaddr",
				"patcherrelativepath" : "../../../../../DOT/branches/v2_max5/toolbox/osc/dot.prependaddr",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "mapperGUI.maxpat",
				"bootpath" : "/Users/jocal/Documents/DOT/branches/v2_max5/mappers/DOTmapper",
				"patcherrelativepath" : "../../../../../DOT/branches/v2_max5/mappers/DOTmapper",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "DOTmapper.js",
				"bootpath" : "/Users/jocal/Documents/DOT/branches/v2_max5/mappers/DOTmapper/components",
				"patcherrelativepath" : "../../../../../DOT/branches/v2_max5/mappers/DOTmapper/components",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "DOTmapper.buttonbar.maxpat",
				"bootpath" : "/Users/jocal/Documents/DOT/branches/v2_max5/mappers/DOTmapper/components",
				"patcherrelativepath" : "../../../../../DOT/branches/v2_max5/mappers/DOTmapper/components",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "DOTmapper.button.maxpat",
				"bootpath" : "/Users/jocal/Documents/DOT/branches/v2_max5/mappers/DOTmapper/components",
				"patcherrelativepath" : "../../../../../DOT/branches/v2_max5/mappers/DOTmapper/components",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "DOTmapper.rangebar.maxpat",
				"bootpath" : "/Users/jocal/Documents/DOT/branches/v2_max5/mappers/DOTmapper/components",
				"patcherrelativepath" : "../../../../../DOT/branches/v2_max5/mappers/DOTmapper/components",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "boundary_icons.png",
				"bootpath" : "/Users/jocal/Documents/Mappers/webmapper/images",
				"patcherrelativepath" : "../../../../webmapper/images",
				"type" : "PNG ",
				"implicit" : 1
			}
, 			{
				"name" : "DOTmapper.browser.maxpat",
				"bootpath" : "/Users/jocal/Documents/DOT/branches/v2_max5/mappers/DOTmapper/components",
				"patcherrelativepath" : "../../../../../DOT/branches/v2_max5/mappers/DOTmapper/components",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "DOTmapper.searchbar.maxpat",
				"bootpath" : "/Users/jocal/Documents/DOT/branches/v2_max5/mappers/DOTmapper/components",
				"patcherrelativepath" : "../../../../../DOT/branches/v2_max5/mappers/DOTmapper/components",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "jcom.ui.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.hub.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.oscroute.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.return.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.init.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.parameter.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.in.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.message.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.in~.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.out~.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.meter~.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.remote.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.dataspace.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.textslider.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.savebang.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.route.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.loader.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.loader.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.cuemanager.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.pass.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.overdrive~.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.limiter~.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.stats.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "mapper.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "jcom.oscinstance.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "oscmulticast.mxo",
				"type" : "iLaX"
			}
 ]
	}

}
