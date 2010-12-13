{
	"patcher" : 	{
		"fileversion" : 1,
		"rect" : [ 120.0, 103.0, 535.0, 445.0 ],
		"bglocked" : 0,
		"defrect" : [ 120.0, 103.0, 535.0, 445.0 ],
		"openrect" : [ 0.0, 0.0, 0.0, 0.0 ],
		"openinpresentation" : 0,
		"default_fontsize" : 12.0,
		"default_fontface" : 0,
		"default_fontname" : "Arial",
		"gridonopen" : 0,
		"gridsize" : [ 15.0, 15.0 ],
		"gridsnaponopen" : 0,
		"toolbarvisible" : 1,
		"boxanimatetime" : 200,
		"imprint" : 0,
		"enablehscroll" : 1,
		"enablevscroll" : 1,
		"devicewidth" : 0.0,
		"boxes" : [ 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p network_neighbourhood",
					"id" : "obj-7",
					"fontname" : "Arial",
					"numinlets" : 0,
					"patching_rect" : [ 30.0, 360.0, 149.0, 20.0 ],
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 35.0, 44.0, 1082.0, 472.0 ],
						"bglocked" : 0,
						"defrect" : [ 35.0, 44.0, 1082.0, 472.0 ],
						"openrect" : [ 0.0, 0.0, 0.0, 0.0 ],
						"openinpresentation" : 0,
						"default_fontsize" : 12.0,
						"default_fontface" : 0,
						"default_fontname" : "Arial",
						"gridonopen" : 0,
						"gridsize" : [ 15.0, 15.0 ],
						"gridsnaponopen" : 0,
						"toolbarvisible" : 1,
						"boxanimatetime" : 200,
						"imprint" : 0,
						"enablehscroll" : 1,
						"enablevscroll" : 1,
						"devicewidth" : 0.0,
						"boxes" : [ 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "t /who clear",
									"id" : "obj-13",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 30.0, 255.0, 283.0, 20.0 ],
									"numoutlets" : 2,
									"fontsize" : 12.0,
									"outlettype" : [ "/who", "clear" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "button",
									"id" : "obj-12",
									"numinlets" : 1,
									"patching_rect" : [ 30.0, 210.0, 36.0, 36.0 ],
									"numoutlets" : 1,
									"outlettype" : [ "bang" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "<- click to update the display",
									"id" : "obj-10",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 75.0, 210.0, 277.0, 20.0 ],
									"numoutlets" : 0,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "coll neighbourhood 1",
									"id" : "obj-5",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 30.0, 390.0, 122.0, 20.0 ],
									"numoutlets" : 4,
									"fontsize" : 12.0,
									"outlettype" : [ "", "", "", "" ],
									"saved_object_attributes" : 									{
										"embed" : 0
									}

								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "prepend store",
									"id" : "obj-4",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 30.0, 345.0, 85.0, 20.0 ],
									"numoutlets" : 1,
									"fontsize" : 12.0,
									"outlettype" : [ "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "route /device",
									"id" : "obj-2",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 30.0, 315.0, 79.0, 20.0 ],
									"numoutlets" : 2,
									"fontsize" : 12.0,
									"outlettype" : [ "", "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "loadmess refer neighbourhood",
									"id" : "obj-1",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 30.0, 30.0, 173.0, 20.0 ],
									"numoutlets" : 1,
									"fontsize" : 12.0,
									"outlettype" : [ "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "jit.cellblock",
									"colwidth" : 90,
									"cols" : 11,
									"id" : "obj-23",
									"fontname" : "Arial",
									"numinlets" : 2,
									"hscroll" : 0,
									"patching_rect" : [ 30.0, 60.0, 1008.0, 132.0 ],
									"numoutlets" : 4,
									"rows" : 5,
									"fontsize" : 12.0,
									"outlettype" : [ "list", "", "", "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "oscmulticast @group 224.0.1.3 @port 7570",
									"id" : "obj-3",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 30.0, 285.0, 242.0, 20.0 ],
									"numoutlets" : 1,
									"fontsize" : 12.0,
									"outlettype" : [ "" ]
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-1", 0 ],
									"destination" : [ "obj-23", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-12", 0 ],
									"destination" : [ "obj-13", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-13", 0 ],
									"destination" : [ "obj-3", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-13", 1 ],
									"destination" : [ "obj-5", 0 ],
									"hidden" : 0,
									"midpoints" : [ 303.5, 377.0, 39.5, 377.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-2", 0 ],
									"destination" : [ "obj-4", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-3", 0 ],
									"destination" : [ "obj-2", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-4", 0 ],
									"destination" : [ "obj-5", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"default_fontface" : 0,
						"fontface" : 0,
						"fontsize" : 12.0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "loadmess refer info",
					"id" : "obj-24",
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 30.0, 150.0, 113.0, 20.0 ],
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "jit.cellblock",
					"colwidth" : 90,
					"cols" : 2,
					"vscroll" : 0,
					"id" : "obj-23",
					"fontname" : "Arial",
					"numinlets" : 2,
					"hscroll" : 0,
					"patching_rect" : [ 30.0, 180.0, 180.0, 90.0 ],
					"numoutlets" : 4,
					"rows" : 5,
					"fontsize" : 12.0,
					"outlettype" : [ "list", "", "", "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "prepend store",
					"id" : "obj-21",
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 225.0, 150.0, 85.0, 20.0 ],
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "coll info 1",
					"id" : "obj-20",
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 225.0, 180.0, 61.0, 20.0 ],
					"numoutlets" : 4,
					"fontsize" : 12.0,
					"outlettype" : [ "", "", "", "" ],
					"saved_object_attributes" : 					{
						"embed" : 0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "r mapper",
					"id" : "obj-1",
					"fontname" : "Arial",
					"numinlets" : 0,
					"patching_rect" : [ 30.0, 90.0, 59.0, 20.0 ],
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p device_definitions",
					"id" : "obj-12",
					"fontname" : "Arial",
					"numinlets" : 0,
					"patching_rect" : [ 150.0, 300.0, 116.0, 20.0 ],
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 709.0, 247.0, 380.0, 333.0 ],
						"bglocked" : 0,
						"defrect" : [ 709.0, 247.0, 380.0, 333.0 ],
						"openrect" : [ 0.0, 0.0, 0.0, 0.0 ],
						"openinpresentation" : 0,
						"default_fontsize" : 12.0,
						"default_fontface" : 0,
						"default_fontname" : "Arial",
						"gridonopen" : 0,
						"gridsize" : [ 15.0, 15.0 ],
						"gridsnaponopen" : 0,
						"toolbarvisible" : 1,
						"boxanimatetime" : 200,
						"imprint" : 0,
						"enablehscroll" : 1,
						"enablevscroll" : 1,
						"devicewidth" : 0.0,
						"boxes" : [ 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "To do: add \"write\" method for storing device definition files based on currect state",
									"linecount" : 2,
									"id" : "obj-5",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 45.0, 240.0, 256.0, 34.0 ],
									"numoutlets" : 0,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "mapper @alias foo @definition tester.json",
									"id" : "obj-3",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 60.0, 165.0, 233.0, 20.0 ],
									"numoutlets" : 2,
									"fontsize" : 12.0,
									"outlettype" : [ "list", "list" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "A JSON-formated device definition file can be specified as an attribute for the mapper object. On instantiation the object will read the file and register the appropriate signals.",
									"linecount" : 4,
									"id" : "obj-2",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 45.0, 45.0, 292.0, 62.0 ],
									"numoutlets" : 0,
									"fontsize" : 12.0
								}

							}
 ],
						"lines" : [  ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"default_fontface" : 0,
						"fontface" : 0,
						"fontsize" : 12.0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p simple_example",
					"id" : "obj-11",
					"fontname" : "Arial",
					"numinlets" : 0,
					"patching_rect" : [ 150.0, 330.0, 107.0, 20.0 ],
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 30.0, 272.0, 553.0, 438.0 ],
						"bglocked" : 0,
						"defrect" : [ 30.0, 272.0, 553.0, 438.0 ],
						"openrect" : [ 0.0, 0.0, 0.0, 0.0 ],
						"openinpresentation" : 0,
						"default_fontsize" : 12.0,
						"default_fontface" : 0,
						"default_fontname" : "Arial",
						"gridonopen" : 0,
						"gridsize" : [ 15.0, 15.0 ],
						"gridsnaponopen" : 0,
						"toolbarvisible" : 1,
						"boxanimatetime" : 200,
						"imprint" : 0,
						"enablehscroll" : 1,
						"enablevscroll" : 1,
						"devicewidth" : 0.0,
						"boxes" : [ 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "route /in1",
									"id" : "obj-10",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 30.0, 195.0, 61.0, 20.0 ],
									"numoutlets" : 2,
									"fontsize" : 12.0,
									"outlettype" : [ "", "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "slider",
									"id" : "obj-8",
									"numinlets" : 1,
									"patching_rect" : [ 30.0, 225.0, 217.0, 35.0 ],
									"numoutlets" : 1,
									"outlettype" : [ "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "slider",
									"id" : "obj-6",
									"numinlets" : 1,
									"patching_rect" : [ 30.0, 30.0, 217.0, 35.0 ],
									"numoutlets" : 1,
									"outlettype" : [ "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "The administrative traffic for the mapping network uses multicast UDP.",
									"linecount" : 2,
									"id" : "obj-22",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 270.0, 360.0, 211.0, 34.0 ],
									"numoutlets" : 0,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Sends the /link and /connect messages which create a mapping connection between the two instances of the mapper object",
									"linecount" : 6,
									"id" : "obj-20",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 90.0, 300.0, 150.0, 89.0 ],
									"numoutlets" : 0,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "---------------------------------->",
									"id" : "obj-14",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 90.0, 285.0, 161.0, 20.0 ],
									"numoutlets" : 0,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "/out1 $1",
									"id" : "obj-18",
									"fontname" : "Arial",
									"numinlets" : 2,
									"patching_rect" : [ 30.0, 75.0, 54.0, 18.0 ],
									"numoutlets" : 1,
									"fontsize" : 12.0,
									"outlettype" : [ "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "mapper @def tester.json",
									"id" : "obj-11",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 30.0, 150.0, 141.0, 20.0 ],
									"numoutlets" : 2,
									"fontsize" : 12.0,
									"outlettype" : [ "list", "list" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "mapper @def tester.json",
									"id" : "obj-9",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 30.0, 105.0, 141.0, 20.0 ],
									"numoutlets" : 2,
									"fontsize" : 12.0,
									"outlettype" : [ "list", "list" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pack s s",
									"id" : "obj-7",
									"fontname" : "Arial",
									"numinlets" : 2,
									"patching_rect" : [ 255.0, 255.0, 64.0, 20.0 ],
									"numoutlets" : 1,
									"fontsize" : 12.0,
									"outlettype" : [ "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "/link $1 $2, /connect $1/out1 $2/in1 @mode expression @expression y=127-x",
									"linecount" : 2,
									"id" : "obj-5",
									"fontname" : "Arial",
									"numinlets" : 2,
									"patching_rect" : [ 255.0, 285.0, 246.0, 32.0 ],
									"numoutlets" : 1,
									"fontsize" : 12.0,
									"outlettype" : [ "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "buddy",
									"id" : "obj-4",
									"fontname" : "Arial",
									"numinlets" : 2,
									"patching_rect" : [ 255.0, 225.0, 64.0, 20.0 ],
									"numoutlets" : 2,
									"fontsize" : 12.0,
									"outlettype" : [ "", "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "oscmulticast @group 224.0.1.3 @port 7570",
									"id" : "obj-3",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 255.0, 330.0, 242.0, 20.0 ],
									"numoutlets" : 1,
									"fontsize" : 12.0,
									"outlettype" : [ "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "route name",
									"id" : "obj-2",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 300.0, 195.0, 71.0, 20.0 ],
									"numoutlets" : 2,
									"fontsize" : 12.0,
									"outlettype" : [ "", "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "route name",
									"id" : "obj-1",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 255.0, 150.0, 71.0, 20.0 ],
									"numoutlets" : 2,
									"fontsize" : 12.0,
									"outlettype" : [ "", "" ]
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-11", 1 ],
									"destination" : [ "obj-2", 0 ],
									"hidden" : 0,
									"midpoints" : [ 161.5, 182.0, 309.5, 182.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-9", 1 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [ 161.5, 137.0, 264.5, 137.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-1", 0 ],
									"destination" : [ "obj-4", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-10", 0 ],
									"destination" : [ "obj-8", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-11", 0 ],
									"destination" : [ "obj-10", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-18", 0 ],
									"destination" : [ "obj-9", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-2", 0 ],
									"destination" : [ "obj-4", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-4", 1 ],
									"destination" : [ "obj-7", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-4", 0 ],
									"destination" : [ "obj-7", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-5", 0 ],
									"destination" : [ "obj-3", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-6", 0 ],
									"destination" : [ "obj-18", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-7", 0 ],
									"destination" : [ "obj-5", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"default_fontface" : 0,
						"fontface" : 0,
						"fontsize" : 12.0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p adding_signals",
					"id" : "obj-10",
					"fontname" : "Arial",
					"numinlets" : 0,
					"patching_rect" : [ 30.0, 300.0, 101.0, 20.0 ],
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 224.0, 558.0, 519.0, 398.0 ],
						"bglocked" : 0,
						"defrect" : [ 224.0, 558.0, 519.0, 398.0 ],
						"openrect" : [ 0.0, 0.0, 0.0, 0.0 ],
						"openinpresentation" : 0,
						"default_fontsize" : 12.0,
						"default_fontface" : 0,
						"default_fontname" : "Arial",
						"gridonopen" : 0,
						"gridsize" : [ 15.0, 15.0 ],
						"gridsnaponopen" : 0,
						"toolbarvisible" : 1,
						"boxanimatetime" : 200,
						"imprint" : 0,
						"enablehscroll" : 1,
						"enablevscroll" : 1,
						"devicewidth" : 0.0,
						"boxes" : [ 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "remove output /pressure",
									"id" : "obj-2",
									"fontname" : "Arial",
									"numinlets" : 2,
									"patching_rect" : [ 150.0, 285.0, 141.0, 18.0 ],
									"numoutlets" : 1,
									"fontsize" : 12.0,
									"outlettype" : [ "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "add output /pressure @type f",
									"id" : "obj-6",
									"fontname" : "Arial",
									"numinlets" : 2,
									"patching_rect" : [ 120.0, 255.0, 165.0, 18.0 ],
									"numoutlets" : 1,
									"fontsize" : 12.0,
									"outlettype" : [ "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "add input /rate @type f @units Hz @min 0 @max 100",
									"id" : "obj-5",
									"fontname" : "Arial",
									"numinlets" : 2,
									"patching_rect" : [ 90.0, 225.0, 297.0, 18.0 ],
									"numoutlets" : 1,
									"fontsize" : 12.0,
									"outlettype" : [ "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Input and ouput signals can be added to the device by sending the message \"add input\" or \"add output\" along with a signal name and data type. Signal metadata is specified using tagged arguments; only the type tag is required but any others can be added. If min and max tags are supplied the system will be able to automatically use linear scaling when creating mapping connections.",
									"linecount" : 5,
									"id" : "obj-4",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 45.0, 45.0, 434.0, 75.0 ],
									"numoutlets" : 0,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "add input /gain @type f @units normalized @min 0 @max 1",
									"id" : "obj-3",
									"fontname" : "Arial",
									"numinlets" : 2,
									"patching_rect" : [ 60.0, 195.0, 330.0, 18.0 ],
									"numoutlets" : 1,
									"fontsize" : 12.0,
									"outlettype" : [ "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "s mapper",
									"id" : "obj-1",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 60.0, 315.0, 61.0, 20.0 ],
									"numoutlets" : 0,
									"fontsize" : 12.0
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-2", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-3", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-5", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-6", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"default_fontface" : 0,
						"fontface" : 0,
						"fontsize" : 12.0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p learn_mode",
					"id" : "obj-9",
					"fontname" : "Arial",
					"numinlets" : 0,
					"patching_rect" : [ 30.0, 330.0, 84.0, 20.0 ],
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 51.0, 186.0, 412.0, 456.0 ],
						"bglocked" : 0,
						"defrect" : [ 51.0, 186.0, 412.0, 456.0 ],
						"openrect" : [ 0.0, 0.0, 0.0, 0.0 ],
						"openinpresentation" : 0,
						"default_fontsize" : 12.0,
						"default_fontface" : 0,
						"default_fontname" : "Arial",
						"gridonopen" : 0,
						"gridsize" : [ 15.0, 15.0 ],
						"gridsnaponopen" : 0,
						"toolbarvisible" : 1,
						"boxanimatetime" : 200,
						"imprint" : 0,
						"enablehscroll" : 1,
						"enablevscroll" : 1,
						"devicewidth" : 0.0,
						"boxes" : [ 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pak /out3 f",
									"id" : "obj-25",
									"fontname" : "Arial",
									"numinlets" : 2,
									"patching_rect" : [ 210.0, 165.0, 67.0, 20.0 ],
									"numoutlets" : 1,
									"fontsize" : 12.0,
									"outlettype" : [ "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pak /out2 i",
									"id" : "obj-24",
									"fontname" : "Arial",
									"numinlets" : 2,
									"patching_rect" : [ 120.0, 165.0, 66.0, 20.0 ],
									"numoutlets" : 1,
									"fontsize" : 12.0,
									"outlettype" : [ "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pak /out1 i",
									"id" : "obj-23",
									"fontname" : "Arial",
									"numinlets" : 2,
									"patching_rect" : [ 30.0, 165.0, 66.0, 20.0 ],
									"numoutlets" : 1,
									"fontsize" : 12.0,
									"outlettype" : [ "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "random 100",
									"id" : "obj-21",
									"fontname" : "Arial",
									"numinlets" : 2,
									"patching_rect" : [ 210.0, 120.0, 75.0, 20.0 ],
									"numoutlets" : 1,
									"fontsize" : 12.0,
									"outlettype" : [ "int" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "random 100",
									"id" : "obj-20",
									"fontname" : "Arial",
									"numinlets" : 2,
									"patching_rect" : [ 120.0, 120.0, 75.0, 20.0 ],
									"numoutlets" : 1,
									"fontsize" : 12.0,
									"outlettype" : [ "int" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "random 100",
									"id" : "obj-19",
									"fontname" : "Arial",
									"numinlets" : 2,
									"patching_rect" : [ 30.0, 120.0, 75.0, 20.0 ],
									"numoutlets" : 1,
									"fontsize" : 12.0,
									"outlettype" : [ "int" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "1) start sending some signals",
									"id" : "obj-18",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 60.0, 45.0, 204.0, 20.0 ],
									"numoutlets" : 0,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "toggle",
									"id" : "obj-16",
									"numinlets" : 1,
									"patching_rect" : [ 30.0, 45.0, 20.0, 20.0 ],
									"numoutlets" : 1,
									"outlettype" : [ "int" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "number",
									"id" : "obj-13",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 135.0, 390.0, 50.0, 20.0 ],
									"numoutlets" : 2,
									"fontsize" : 12.0,
									"outlettype" : [ "int", "bang" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "route numOutputs",
									"id" : "obj-11",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 135.0, 360.0, 107.0, 20.0 ],
									"numoutlets" : 2,
									"fontsize" : 12.0,
									"outlettype" : [ "", "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "metro 250",
									"id" : "obj-10",
									"fontname" : "Arial",
									"numinlets" : 2,
									"patching_rect" : [ 30.0, 75.0, 65.0, 20.0 ],
									"numoutlets" : 1,
									"fontsize" : 12.0,
									"outlettype" : [ "bang" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "3) watch the number of reported signals",
									"linecount" : 2,
									"id" : "obj-7",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 195.0, 390.0, 150.0, 34.0 ],
									"numoutlets" : 0,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "2) turn on learn mode",
									"id" : "obj-6",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 135.0, 240.0, 150.0, 20.0 ],
									"numoutlets" : 0,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "learn 0",
									"id" : "obj-4",
									"fontname" : "Arial",
									"numinlets" : 2,
									"patching_rect" : [ 75.0, 270.0, 47.0, 18.0 ],
									"numoutlets" : 1,
									"fontsize" : 12.0,
									"outlettype" : [ "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "learn 1",
									"id" : "obj-3",
									"fontname" : "Arial",
									"numinlets" : 2,
									"patching_rect" : [ 75.0, 240.0, 47.0, 18.0 ],
									"numoutlets" : 1,
									"fontsize" : 12.0,
									"outlettype" : [ "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "mapper",
									"id" : "obj-1",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 30.0, 300.0, 124.0, 20.0 ],
									"numoutlets" : 2,
									"fontsize" : 12.0,
									"outlettype" : [ "list", "list" ]
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-1", 1 ],
									"destination" : [ "obj-11", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-10", 0 ],
									"destination" : [ "obj-19", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-10", 0 ],
									"destination" : [ "obj-20", 0 ],
									"hidden" : 0,
									"midpoints" : [ 39.5, 107.0, 129.5, 107.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-10", 0 ],
									"destination" : [ "obj-21", 0 ],
									"hidden" : 0,
									"midpoints" : [ 39.5, 107.0, 219.5, 107.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-11", 0 ],
									"destination" : [ "obj-13", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-16", 0 ],
									"destination" : [ "obj-10", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-19", 0 ],
									"destination" : [ "obj-23", 1 ],
									"hidden" : 0,
									"midpoints" : [ 39.5, 152.0, 86.5, 152.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-20", 0 ],
									"destination" : [ "obj-24", 1 ],
									"hidden" : 0,
									"midpoints" : [ 129.5, 152.0, 176.5, 152.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-21", 0 ],
									"destination" : [ "obj-25", 1 ],
									"hidden" : 0,
									"midpoints" : [ 219.5, 152.0, 267.5, 152.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-23", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-24", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [ 129.5, 197.0, 39.5, 197.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-25", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [ 219.5, 197.0, 39.5, 197.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-3", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-4", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"default_fontface" : 0,
						"fontface" : 0,
						"fontsize" : 12.0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "The mapper object forms part of a distributed network of devices communicating over the local network. The inputs and outputs of each device can be discovered and queried by other participants in the network, and mapping connections can be created, edited, and destroyed using messages or graphical interfaces.",
					"linecount" : 12,
					"id" : "obj-8",
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 315.0, 90.0, 172.0, 172.0 ],
					"numoutlets" : 0,
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "mapper @alias my_device",
					"id" : "obj-6",
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 30.0, 120.0, 214.0, 20.0 ],
					"numoutlets" : 2,
					"fontsize" : 12.0,
					"outlettype" : [ "list", "list" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "For more information visit www.idmil.org/software/libmapper",
					"linecount" : 2,
					"id" : "obj-3",
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 30.0, 390.0, 189.0, 34.0 ],
					"numoutlets" : 0,
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "bpatcher",
					"id" : "obj-64",
					"args" : [  ],
					"numinlets" : 0,
					"patching_rect" : [ 315.0, 285.0, 211.0, 145.0 ],
					"numoutlets" : 0,
					"name" : "dot.menu.maxpat"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "A MaxMSP wrapper for libmapper/Digital Orchestra Tools",
					"id" : "obj-16",
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 15.0, 45.0, 484.0, 23.0 ],
					"numoutlets" : 0,
					"fontsize" : 14.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"varname" : "autohelp_top_title",
					"text" : "mapper",
					"id" : "obj-22",
					"fontname" : "Arial",
					"numinlets" : 1,
					"frgb" : [ 1.0, 1.0, 1.0, 1.0 ],
					"patching_rect" : [ 15.0, 15.0, 485.0, 30.0 ],
					"numoutlets" : 0,
					"textcolor" : [ 1.0, 1.0, 1.0, 1.0 ],
					"fontface" : 3,
					"fontsize" : 20.871338
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "panel",
					"id" : "obj-60",
					"numinlets" : 1,
					"patching_rect" : [ 501.0, 15.0, 4.0, 304.0 ],
					"numoutlets" : 0,
					"bgcolor" : [ 0.0, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "panel",
					"varname" : "autohelp_top_panel[1]",
					"background" : 1,
					"grad1" : [ 0.0, 0.0, 0.0, 1.0 ],
					"grad2" : [ 1.0, 0.0, 0.0, 1.0 ],
					"id" : "obj-2",
					"numinlets" : 1,
					"angle" : 180.0,
					"patching_rect" : [ 15.0, 15.0, 490.0, 31.0 ],
					"mode" : 1,
					"numoutlets" : 0
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"source" : [ "obj-6", 1 ],
					"destination" : [ "obj-21", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-1", 0 ],
					"destination" : [ "obj-6", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-21", 0 ],
					"destination" : [ "obj-20", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-24", 0 ],
					"destination" : [ "obj-23", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
 ]
	}

}
