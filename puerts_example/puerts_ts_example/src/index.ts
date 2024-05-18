import { load } from "puerts";
import * as HelloWorldModlue from "hello_world";

let hello_world = load<typeof HelloWorldModlue>("../hello_world/hello_world");

const HelloWorld = hello_world.HelloWorld;

const obj = new HelloWorld(101);

obj.Foo((x, y) => x > y);

HelloWorld.Bar("hello");

HelloWorld.StaticField = 999;
obj.Field = 888;

obj.Foo((x, y) => x > y);
