# mySelf

My implementation of Self.

## What?

[Self](https://selflanguage.org/) is an object oriented programming language and
environment similar to Smalltalk, featuring a prototype-based inheritance system.
The language originates from Xerox PARC and was later developed at Sun Microsystems.

My goal is to modernize the language, the module system and the programming
environment. Because no significant work has been done on the original
programming environment in quite a while, it is pretty dated. I believe the
ideas in Self are worthwhile and want to see how it would fare among today's
programming languages.

## Current status

- [x] Parser
- [ ] Object system
- [ ] Garbage collection
- [ ] Runtime (Milestone 1)
- [ ] Shell (Milestone 2)
- [ ] Standard library (Milestone 3)
- [ ] Programming environment and modernization of Morphic (Milestone 4)

## Notable changes

- Object annotations are now restricted to module, category and comment. The
  syntax is now (subject to change):

  ```self
  {} = 'Object comment'.
  {} = <-'Object module'.

  {
      'Comment in annotation block'.
      *'Category for the slots in this block'.
      <-'Module for the slots in this block'.
  }
  ```

- The module system no longer uses the Transporter system as it generates quite
  ugly module files, in my opinion. A new system is to be implemented.

## Contributing

Big changes should be discussed in an issue beforehand. Code style is dictated
by clang-format (format file to be added).

## License

&copy; 2020 syn-ack. This repository is licensed under the GNU General Public
License version 3.
