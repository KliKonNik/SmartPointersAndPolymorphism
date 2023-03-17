// SmartPointersAndPolymorphism.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
//
//  Описание задачи:
//      Имеется метод в Server\Server.Core\Server.Core.Static\ServerBuilder.hpp:
//      namespace Server::Builder
//      {
//          ServerBuilder::Server* make() noexcept
//          {
//              // Here it generates std::string dbOptions. How it happens is not matter.
//          
//              auto server = new Server();
//          
//              if (!_repository)
//              {
//                  // Here: class PostgreRepositoryManager: public IRepositoryManager
//                  auto repoManager = std::make_unique<DataAccess::PostgreRepositoryManager>(
//                      DataAccess::IAdapter::getInstance<DataAccess::PostgreAdapter>(dbOptions));
//                  server->initRepository(std::move(repoManager));
//              }
//              else
//              {
//                  server->initRepository(std::move(_repository)); // assume: std::unique_ptr<DataAccess::IRepositoryManager> argument type
//              }
//              
//              // Here and below, we initialize connection port and other operations
//              
//              return server;
//          }
//      }  /// namespace Server::Builder
//      
//      Надо:
//      Разработать интерфейс для организации универсального способа передачи данных подключения к БД (для различных предков
//      IRepositoryManager) и возврата самого подключения. В текущем контексте имеется подключение только к Postgre.
//      Есть:      
//      новый файл Server\Server.Core\Server.Core.Static\AbstractServerBuilder.hpp:
//      namespace Server::Builder
//      {
//          class IServerBuilder
//          {
//          public:
//              IServerBuilder() = default;
//              virtual ~IServerBuilder() {}
//              virtual std::unique_ptr<DataAccess::IRepositoryManager> GetConnectionOptions(std::string options) = 0;
//          };
//      }  /// namespace Server::Builder
// 
//      Изменённый метод в файле Server\Server.Core\Server.Core.Static\ServerBuilder.hpp:
//      namespace Server::Builder
//      {
//          ServerBuilder::Server* make() noexcept
//          {
//              // Here it generates std::string dbOptions. How it happens is not matter.
//          
//              auto server = new Server();
//          
//              if (!_repository)
//              {
//                  // Here: class PostgreRepositoryManager: public IRepositoryManager
//                  std::unique_ptr<IServerBuilder> repoManager = std::make_unique<DataAccess::PostgreRepositoryManager>();
//                  server->initRepository(std::move(repoManager->GetConnectionOptions(dbOptions)));
//              }
//              else
//              {
//                  server->initRepository(std::move(_repository)); // assume: std::unique_ptr<DataAccess::IRepositoryManager> argument type
//              }
//              
//              // Here and below, we initialize connection port and other operations
//              return server;
//          }
//      }  /// namespace Server::Builder
// 
//      Изменённый файл DataAccess\DataAccess.Postgre\PostgreRepositoryManager.hpp:
//      namespace DataAccess
//      {
//           class PostgreRepositoryManager : public IRepositoryManager, public Server::Builder::IServerBuilder
//           {
//           public:
//               PostgreRepositoryManager() {}
//      
//               std::unique_ptr<DataAccess::IRepositoryManager> GetConnectionOptions(std::string options)
//               {
//                   auto container = std::make_shared<PostgreAdapter>(options);
//                   auto repo = std::make_unique<AbstractRepositoryContainer>(container);
//                   repo->registerRepository<IChannelsRepository, ChannelsRepository>();
//                   repo->registerRepository<ILoginRepository, LoginRepository>();
//                   repo->registerRepository<IMessagesRepository, MessagesRepository>();
//                   repo->registerRepository<IRegisterRepository, RegisterRepository>();
//                   repo->registerRepository<IRepliesRepository, RepliesRepository>();
//                   repo->registerRepository<IDirectMessageRepository, DirectMessageRepository>();
//      
//                   IRepositoryManager::init(std::move(repo));
//      
//                   return this; <<------------------ here the trouble
//               }
//           };
//      }  /// namespace DataAccess
//
//      В настоящем файле проводится исследование проблемной ситуации и путей к ней приводящих, а также некоторые варианты альтернативных
//      реализаций работающих без проблем
//      Для упрощения тестирования примем следующие сокращения:
//          Оригильное имя класса           --->    Псевдоним
//          PostgreRepositoryManager        --->    SpecificRM
//          IRepositoryManager              --->    IRM
//          IServerBuilder                  --->    ISB
//          server->initRepository()        --->    initRep()
//

#include <iostream>
#include <string>

using std::cout;
using std::endl;

/**/
namespace rawPointers
{
    struct IRM
    {
        std::string Get()
        {
            return _connection;
        }
        std::string _connection;
    };

    struct ISB
    {
        virtual ~ISB() = default;
        virtual IRM* GetConnectionOptions(std::string options) = 0;
    };

    struct SpecificRM : public IRM, public ISB
    {
        IRM* GetConnectionOptions(std::string options)
        {
            _connection = options;
            return this;
        }
    };

    void initRep(IRM* rep)
    {
        cout << "rawPointers: " << rep->Get() << endl;
    }

}  /// namespace rawPointers
/**/

/** /
namespace uniquePointersAndSingleInheritance
{
    struct IRM
    {
        std::string _connection;
        virtual std::unique_ptr<IRM> GetConnectionOptions(std::string options){};
    };

    struct ISB
    {
        virtual ~ISB() = default;
        virtual std::unique_ptr<IRM> GetConnectionOptions(std::string options) = 0;
    };

    struct SpecificRM : public IRM
    {
        std::unique_ptr<IRM> GetConnectionOptions(std::string options)
        {
            _connection = options;

            //return std::make_unique<IRM>(this);     // Error C2665: 'uniquePointersAndSingleInheritance::IRM::IRM':
              // no overloaded function could convert all the argument types
              // C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.35.32215\include\memory (line 3393)

            //return *this;                         // Error C2440: 'return':
              // cannot convert from 'uniquePointersAndSingleInheritance::SpecificRM' to
              //'std::unique_ptr<uniquePointersAndSingleInheritance::IRM,std::default_delete<uniquePointersAndSingleInheritance::IRM>>'

            //return this;                          // Error C2440: 'return':
              // cannot convert from 'uniquePointersAndSingleInheritance::SpecificRM *' to
              //'std::unique_ptr<uniquePointersAndSingleInheritance::IRM,std::default_delete<uniquePointersAndSingleInheritance::IRM>>'

            //return std::move(this);               // Error C2440: 'return':
              // cannot convert from 'uniquePointersAndSingleInheritance::SpecificRM *' to
              //'std::unique_ptr<uniquePointersAndSingleInheritance::IRM,std::default_delete<uniquePointersAndSingleInheritance::IRM>>'

            //return std::move(*this);              // Error C2440: 'return':
              // cannot convert from 'uniquePointersAndSingleInheritance::SpecificRM' to
              //'std::unique_ptr<uniquePointersAndSingleInheritance::IRM,std::default_delete<uniquePointersAndSingleInheritance::IRM>>'
        }
    };

    void initRep(std::unique_ptr<IRM> rep)
    {
        cout << "uniquePointersAndSingleInheritance: " << rep->_connection << endl;
    }

}  /// namespace uniquePointersAndSingleInheritance
/**/

/** /
namespace uniquePointersAndSequentialInheritance
{
    struct IRM;

    struct ISB
    {
        virtual ~ISB() = default;
        virtual std::unique_ptr<IRM> GetConnectionOptions(std::string options) = 0;
    };

    struct IRM : public ISB
    {
        std::string _connection;
        virtual std::unique_ptr<IRM> GetConnectionOptions(std::string options){};
    };

    struct SpecificRM : public IRM
    {
        std::unique_ptr<IRM> GetConnectionOptions(std::string options)
        {
            _connection = options;

            return std::make_unique<IRM>(this);     // Error C2665: 'uniquePointersAndSequentialInheritance::IRM::IRM':
              // no overloaded function could convert all the argument types
              // C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.35.32215\include\memory (line 3393)
            
            //return *this;                         // Error C2440: 'return':
              // cannot convert from 'uniquePointersAndSequentialInheritance::SpecificRM' to
              //'std::unique_ptr<uniquePointersAndSequentialInheritance::IRM,std::default_delete<uniquePointersAndSequentialInheritance::IRM>>'
            
            //return this;                          // Error C2440: 'return':
              // cannot convert from 'uniquePointersAndSequentialInheritance::SpecificRM *' to
              //'std::unique_ptr<uniquePointersAndSequentialInheritance::IRM,std::default_delete<uniquePointersAndSequentialInheritance::IRM>>'
            
            //return std::move(this);               // Error C2440: 'return':
              // cannot convert from 'uniquePointersAndSequentialInheritance::SpecificRM *' to
              //'std::unique_ptr<uniquePointersAndSequentialInheritance::IRM,std::default_delete<uniquePointersAndSequentialInheritance::IRM>>'
            
            //return std::move(*this);              // Error C2440: 'return':
            // cannot convert from 'uniquePointersAndSequentialInheritance::SpecificRM' to
              //'std::unique_ptr<uniquePointersAndSequentialInheritance::IRM,std::default_delete<uniquePointersAndSequentialInheritance::IRM>>'
        }
    };

    void initRep(std::unique_ptr<IRM> rep)
    {
        cout << "uniquePointersAndSequentialInheritance: " << rep->_connection << endl;
    }

}  /// namespace uniquePointersAndSequentialInheritance
/**/

/** /
namespace uniquePointersAndParallelInheritance
{
    struct IRM
    {
        std::string _connection;
    };

    struct ISB
    {
        virtual ~ISB() = default;
        virtual std::unique_ptr<IRM> GetConnectionOptions(std::string options) = 0;
    };

    struct SpecificRM : public IRM, public ISB
    {
        std::unique_ptr<IRM> GetConnectionOptions(std::string options)
        {
            _connection = options;

            return std::make_unique<IRM>(this);   // Error C2665: 'uniquePointersAndParallelInheritance::IRM::IRM':
                //no overloaded function could convert all the argument types
                // C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.35.32215\include\memory (line 3393)
            
            //return *this;                         // Error C2440: 'return':
                // cannot convert from 'uniquePointersAndParallelInheritance::SpecificRM' to
                //'std::unique_ptr<uniquePointersAndParallelInheritance::IRM,std::default_delete<uniquePointersAndParallelInheritance::IRM>>'
            
            //return this;                          // Error C2440: 'return':
                // cannot convert from 'uniquePointersAndParallelInheritance::SpecificRM *' to
                //'std::unique_ptr<uniquePointersAndParallelInheritance::IRM,std::default_delete<uniquePointersAndParallelInheritance::IRM>>'
            
            //return std::move(this);               // Error C2440: 'return':
                // cannot convert from 'uniquePointersAndParallelInheritance::SpecificRM *' to
                //'std::unique_ptr<uniquePointersAndParallelInheritance::IRM,std::default_delete<uniquePointersAndParallelInheritance::IRM>>'
            
            //return std::move(*this);              // Error C2440: 'return':
                // cannot convert from 'uniquePointersAndParallelInheritance::SpecificRM' to
                //'std::unique_ptr<uniquePointersAndParallelInheritance::IRM,std::default_delete<uniquePointersAndParallelInheritance::IRM>>'
        }
    };

    void initRep(std::unique_ptr<IRM> rep)
    {
        cout << "uniquePointersAndParallelInheritance: " << rep->_connection << endl;
    }

}  /// namespace uniquePointersAndParallelInheritance
/**/

/** /
namespace possibleSolution
{
    struct IRM
    {
        std::string Get()
        {
            return _connection;
        }
        std::string _connection;
    };

    struct ISB
    {
        virtual ~ISB() = default;
        virtual void GetConnectionOptions(std::string options) = 0;
    };

    struct SpecificRM : public IRM, public ISB
    {
        void GetConnectionOptions(std::string options)
        {
            _connection = options;
        }
    };

    void initRep(std::unique_ptr<IRM> rep)
    {
        cout << "possibleSolution: " << rep->Get() << endl;
    }

}  /// namespace possibleSolution 
/**/

/**/
namespace possibleSolution2
{
    struct ISB
    {
        virtual ~ISB() = default;
        virtual void GetConnectionOptions(std::string options) = 0;
    };

    struct IRM : public ISB
    {
        std::string Get()
        {
            return _connection;
        }
        std::string _connection{"test: "};
        virtual void GetConnectionOptions(std::string options) = 0;
    };

    struct SpecificRM : public IRM
    {
        void GetConnectionOptions(std::string options)
        {
            _connection = options;
        }
    };

    void initRep(std::unique_ptr<IRM> rep)
    {
        cout << "possibleSolution2: " << rep->Get() << endl;
    }

}  /// namespace possibleSolution2
/**/

int main()
{
    if (true)
    {
        using namespace rawPointers;
    
        ISB* universalInterface = new SpecificRM();
        initRep(std::move(universalInterface->GetConnectionOptions("test GOOD on line " + std::to_string(__LINE__))));
    }

    /** /
    if(true)
    {
        using namespace uniquePointersAndSingleInheritance;

        std::unique_ptr<IRM> universalInterface = std::make_unique<SpecificRM>();
        initRep(std::move(universalInterface->GetConnectionOptions("test GOOD on line " + std::to_string(__LINE__))));
    }
    /**/

    /** /
    if(true)
    {
        using namespace uniquePointersAndSequentialInheritance;

        std::unique_ptr<ISB> universalInterface = std::make_unique<SpecificRM>();
        initRep(std::move(universalInterface->GetConnectionOptions("test GOOD on line " + std::to_string(__LINE__))));
    }
    /**/

    /** /
    if (true)
    {
        using namespace uniquePointersAndParallelInheritance;

        std::unique_ptr<ISB> universalInterface = std::make_unique<SpecificRM>();
        initRep(std::move(universalInterface->GetConnectionOptions("test GOOD on line " + std::to_string(__LINE__))));
    }
    /**/

    /** /
    if(true)
    {
        using namespace possibleSolution;

        std::unique_ptr<ISB> universalInterface = std::make_unique<SpecificRM>();
        universalInterface->GetConnectionOptions("test GOOD on line " + std::to_string(__LINE__));
        IRM* pIRM = dynamic_cast<IRM*>(std::move(universalInterface.release()));
        std::unique_ptr<IRM> upIRM {std::move(pIRM)};
        initRep(std::move(upIRM));
        // double destructor error
    }
    /**/

    /**/
    if(true)
    {
        using namespace possibleSolution2;

        std::unique_ptr<IRM> universalInterface = std::make_unique<SpecificRM>();
        universalInterface->GetConnectionOptions("test GOOD on line " + std::to_string(__LINE__));
        initRep(std::move(universalInterface));

    }
    /**/

    return 0;
}
