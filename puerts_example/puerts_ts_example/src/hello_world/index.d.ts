declare module "hello_world" {
    import {$Ref, $Nullable, cstring} from "puerts"

    class HelloWorld {
        constructor(p0: number);
        Field: number;
        static StaticField: number;
        static Bar(p0: string) :number;
        Foo(p0: (p0:number, p1:number) => boolean) :void;
    }

}
